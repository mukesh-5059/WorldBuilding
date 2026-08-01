#include "LandmassGenerator.hpp"
#include "helper.hpp"
#include <cmath>
#include <algorithm>
#include <queue>

LandmassGenerator::LandmassGenerator() {
    RunFullPipeline();
}

LandmassGenerator::~LandmassGenerator() {
}

void LandmassGenerator::RunFullPipeline() {
    RunPlateAssignmentFloodFill();
    CalculateBorders();
    GenerateContinentalLandmassSeeds();
    GenerateContinentalHeightmapData();
}

/* =========================================================================
   Step 2: 2D Plate Assignment via Priority Queue (Grid-Resolution Invariant)
   ========================================================================= */

void LandmassGenerator::RunPlateAssignmentFloodFill() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    int totalCells = w * h;

    cellPlateOwner.assign(totalCells, -1);
    cellPlateDist.assign(totalCells, 1e9f);
    plateTypes.clear();
    plateSeedCells.clear();
    plateGrowthBias.clear();

    int numP = std::clamp(config.numPlates, 2, 50);

    // 1. Generate plate growth step bias and Plate Types (CONTINENTAL vs OCEANIC) using constant colors
    int continentalCount = 0;
    for (int p = 0; p < numP; ++p) {
        float hBias = HashRndId(p, 40, config.seed);
        float gBias = std::max(0.15f, 1.0f + (hBias * 2.0f - 1.0f) * config.plateSizeVariance);
        plateGrowthBias.push_back(gBias);

        PlateType pType = (HashRndId(p, 60, config.seed) < config.landRatio) ? PlateType::CONTINENTAL : PlateType::OCEANIC;
        plateTypes.push_back(pType);
        if (pType == PlateType::CONTINENTAL) continentalCount++;
    }

    if (continentalCount == 0 && numP > 0) {
        plateTypes[0] = PlateType::CONTINENTAL;
    } else if (continentalCount == numP && numP > 1) {
        plateTypes[numP - 1] = PlateType::OCEANIC;
    }

    // 2. Pick random seed cell locations spread across UV space and push to Priority Queue
    std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> pq;

    for (int p = 0; p < numP; ++p) {
        int sx = std::clamp((int)(HashRndId(p, 1, config.seed) * (float)w), 0, w - 1);
        int sy = std::clamp((int)(HashRndId(p, 2, config.seed) * (float)h), 0, h - 1);

        int seedCell = sy * w + sx;
        if (cellPlateOwner[seedCell] == -1) {
            cellPlateOwner[seedCell] = p;
            cellPlateDist[seedCell] = 0.0f;
            plateSeedCells.push_back(seedCell);
            pq.push(PQElement{ 0.0f, seedCell, p });
        }
    }

    // 3. Scale-Invariant Priority Queue Dijkstra expansion across 4-neighbors
    static const int dx[4] = { -1, 1, 0, 0 };
    static const int dy[4] = { 0, 0, -1, 1 };
    float stepScale = 256.0f / (float)w;

    while (!pq.empty()) {
        PQElement top = pq.top();
        pq.pop();

        if (top.dist > cellPlateDist[top.cellIndex]) continue;

        int cx = top.cellIndex % w;
        int cy = top.cellIndex / w;
        float pBias = plateGrowthBias[top.plateId];

        for (int dir = 0; dir < 4; ++dir) {
            int nx = cx + dx[dir];
            int ny = cy + dy[dir];

            if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                int nIdx = ny * w + nx;
                float jitter = 0.95f + HashRndUV(nx, ny, 50, config.seed, w, h) * 0.10f;
                float newDist = top.dist + (1.0f / pBias) * stepScale * jitter;

                if (newDist < cellPlateDist[nIdx]) {
                    cellPlateDist[nIdx] = newDist;
                    cellPlateOwner[nIdx] = top.plateId;
                    pq.push(PQElement{ newDist, nIdx, top.plateId });
                }
            }
        }
    }
}

Image LandmassGenerator::GeneratePlateMapImage() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    Image img = GenImageColor(w, h, BLACK);

    for (int i = 0; i < w * h; ++i) {
        int owner = cellPlateOwner[i];
        Color c = (owner >= 0 && owner < (int)plateTypes.size()) ? GetPlateColor(plateTypes[owner]) : BLACK;
        ImageDrawPixel(&img, i % w, i / w, c);
    }

    if (config.drawSeedPoints) {
        for (int seedCell : plateSeedCells) {
            DrawCrossMarker(&img, seedCell % w, seedCell / w, WHITE, w, h);
        }
    }
    return img;
}

/* =========================================================================
   Step 3: Calculate Continental Interior SDF & Detect Borders (Single Pass)
   ========================================================================= */

void LandmassGenerator::CalculateBorders() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    int totalCells = w * h;

    cellSDF.assign(totalCells, 0.0f);
    cellIsBoundary.assign(totalCells, false);
    continentalBoundaryCellIds.clear();

    cellIsPlateBoundary.assign(totalCells, false);
    allPlateBoundaryCellIds.clear();

    std::queue<int> q;
    static const int dx[4] = { -1, 1, 0, 0 };
    static const int dy[4] = { 0, 0, -1, 1 };
    float stepScale = 256.0f / (float)w;

    // Single pass to detect both inter-plate borders and continental coastline borders
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int cellId = y * w + x;
            int owner = cellPlateOwner[cellId];
            bool touchesOcean = false;

            for (int dir = 0; dir < 4; ++dir) {
                int nx = x + dx[dir];
                int ny = y + dy[dir];

                if (nx < 0 || nx >= w || ny < 0 || ny >= h) {
                    if (IsContinental(owner, plateTypes)) touchesOcean = true;
                    cellIsPlateBoundary[cellId] = true;
                } else {
                    int nIdx = ny * w + nx;
                    int nOwner = cellPlateOwner[nIdx];
                    if (nOwner != owner) {
                        cellIsPlateBoundary[cellId] = true;
                    }
                    if (IsContinental(owner, plateTypes) && IsOceanic(nOwner, plateTypes)) {
                        touchesOcean = true;
                    }
                }
            }

            if (cellIsPlateBoundary[cellId]) {
                allPlateBoundaryCellIds.push_back(cellId);
            }

            if (touchesOcean) {
                cellIsBoundary[cellId] = true;
                continentalBoundaryCellIds.push_back(cellId);
                cellSDF[cellId] = 1.0f;
                q.push(cellId);
            } else if (IsContinental(owner, plateTypes)) {
                cellSDF[cellId] = 1e9f;
            }
        }
    }

    // Multi-Source BFS Distance Propagation Inward for Continental SDF
    while (!q.empty()) {
        int curr = q.front();
        q.pop();

        int cx = curr % w;
        int cy = curr / w;
        float currentDist = cellSDF[curr];

        for (int dir = 0; dir < 4; ++dir) {
            int nx = cx + dx[dir];
            int ny = cy + dy[dir];

            if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                int nIdx = ny * w + nx;
                if (IsContinental(cellPlateOwner[nIdx], plateTypes) && cellSDF[nIdx] > currentDist + stepScale) {
                    cellSDF[nIdx] = currentDist + stepScale;
                    q.push(nIdx);
                }
            }
        }
    }

    // SDF Bias Modulation: 2D Layered Map Gradient vs Low-Frequency Noise
    if (config.sdfBiasMode == SDFBiasMode::MapGradient) {
        if (config.plateTaperStrengthV > 0.001f || config.plateTaperStrengthH > 0.001f) {
            for (int y = 0; y < h; ++y) {
                float v = (float)y / (float)h; // [0.0 (North), 1.0 (South)]
                float taperV = 1.0f - config.plateTaperStrengthV * (v - 0.5f) * 2.0f;

                for (int x = 0; x < w; ++x) {
                    float u = (float)x / (float)w; // [0.0 (West), 1.0 (East)]
                    float taperH = 1.0f - config.plateTaperStrengthH * (u - 0.5f) * 2.0f;
                    float taper = std::clamp(taperV * taperH, 0.10f, 2.0f);

                    int cellId = y * w + x;
                    if (IsContinental(cellPlateOwner[cellId], plateTypes)) {
                        cellSDF[cellId] *= taper;
                    }
                }
            }
        }
    } else if (config.sdfBiasMode == SDFBiasMode::LowFreqNoise) {
        FastNoiseLite lowNoise;
        lowNoise.SetSeed(config.seed + 999);
        lowNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

        float noiseScale = config.lowFreqNoiseFrequency * 2.0f;

        for (int y = 0; y < h; ++y) {
            float v = (float)y / (float)h;
            for (int x = 0; x < w; ++x) {
                float u = (float)x / (float)w;
                float nVal = lowNoise.GetNoise(u * noiseScale * 10.0f, v * noiseScale * 10.0f);
                float bias = std::clamp(1.0f + config.lowFreqNoiseStrength * nVal, 0.10f, 2.0f);

                int cellId = y * w + x;
                if (IsContinental(cellPlateOwner[cellId], plateTypes)) {
                    cellSDF[cellId] *= bias;
                }
            }
        }
    }
}

Image LandmassGenerator::GeneratePlateSDFMapImage() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    Image img = GenImageColor(w, h, BLACK);

    Color oceanBaseColor     = Color{ 21, 101, 192, 255 };  // Deep Ocean Blue
    Color boundaryYellow     = Color{ 255, 235, 59, 255 };  // Bright Coastline Boundary Yellow
    Color plateBoundaryColor = Color{ 255, 87, 34, 255 };   // Inter-plate boundary Orange/Red
    Color landmassSeedColor  = Color{ 233, 30, 99, 255 };   // Landmass seed Magenta

    float maxDepth = std::max(1.0f, config.maxSDFDepth);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int cellId = y * w + x;
            int owner = cellPlateOwner[cellId];
            Color c;

            if (IsOceanic(owner, plateTypes)) {
                c = oceanBaseColor;
            } else {
                float t = std::clamp(cellSDF[cellId] / maxDepth, 0.0f, 1.0f);
                if (!config.drawSDFColors) {
                    c = Color{ 46, 125, 50, 255 };
                } else {
                    c = Color{
                        (unsigned char)((1.0f - t) * 140.0f + t * 15.0f),
                        (unsigned char)((1.0f - t) * 210.0f + t * 85.0f),
                        (unsigned char)((1.0f - t) * 90.0f  + t * 30.0f),
                        255
                    };
                }
            }
            ImageDrawPixel(&img, x, y, c);
        }
    }

    if (config.drawPlateBoundaries) {
        for (int pbCell : allPlateBoundaryCellIds) {
            ImageDrawPixel(&img, pbCell % w, pbCell / w, plateBoundaryColor);
        }
    }

    if (config.drawBoundaries) {
        for (int bCell : continentalBoundaryCellIds) {
            ImageDrawPixel(&img, bCell % w, bCell / w, boundaryYellow);
        }
    }

    if (config.drawSeedPoints) {
        for (int seedCell : plateSeedCells) {
            DrawCrossMarker(&img, seedCell % w, seedCell / w, WHITE, w, h);
        }
    }

    if (config.drawLandmassSeeds) {
        for (const auto& sp : continentalLandmassSeeds) {
            int lx = (int)(((sp.x + 1.0f) * 0.5f) * (float)w);
            int ly = (int)(((sp.y + 1.0f) * 0.5f) * (float)h);
            DrawCrossMarker(&img, lx, ly, landmassSeedColor, w, h);
        }
    }

    return img;
}

/* =========================================================================
   Step 4: Generate Continental Adaptive Landmass Seeds & Heightmap
   ========================================================================= */

void LandmassGenerator::GenerateContinentalLandmassSeeds() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    continentalLandmassSeeds.clear();

    FalloffType availableShapes[] = { FalloffType::Radial, FalloffType::Diamond, FalloffType::Starfish };
    int numAvailable = 3;
    int minArms = 3;
    int maxArms = std::max(3, (int)roundf(HARDCODED_STARFISH_ARMS));
    int armsRange = std::max(1, maxArms - minArms + 1);

    // 1. Gather candidate continental cells with SDF >= minInteriorSDF
    std::vector<int> candidates;
    for (int cellId = 0; cellId < w * h; ++cellId) {
        if (IsContinental(cellPlateOwner[cellId], plateTypes) && cellSDF[cellId] >= config.minInteriorSDF) {
            candidates.push_back(cellId);
        }
    }

    if (candidates.empty()) return;

    // 2. Sort candidate cells by SDF depth weighted with concentrationPower
    float concPow = std::clamp(config.concentrationPower, 0.1f, 5.0f);

    std::sort(candidates.begin(), candidates.end(), [this, concPow, w, h](int a, int b) {
        float scoreA = powf(cellSDF[a], concPow) * (0.85f + HashRndUV(a % w, a / w, 90, config.seed, w, h) * 0.30f);
        float scoreB = powf(cellSDF[b], concPow) * (0.85f + HashRndUV(b % w, b / w, 90, config.seed, w, h) * 0.30f);
        return scoreA > scoreB; // Descending order: deepest interior middle cells first!
    });

    // 3. Scale-Invariant Poisson-Disk sampling to place adaptive seeds
    int seedIdx = 0;
    float minSpacing = std::max(4.0f, config.seedSpacing * ((float)w / 256.0f));

    for (int cellId : candidates) {
        int cx = cellId % w;
        int cy = cellId / w;

        bool valid = true;
        for (const auto& sp : continentalLandmassSeeds) {
            float sx = ((sp.x + 1.0f) * 0.5f) * (float)w;
            float sy = ((sp.y + 1.0f) * 0.5f) * (float)h;
            if (sqrtf((cx - sx) * (cx - sx) + (cy - sy) * (cy - sy)) < minSpacing) {
                valid = false;
                break;
            }
        }

        if (valid) {
            float localSDFRef = cellSDF[cellId];
            float rNorm = std::clamp(config.radiusScale * (localSDFRef / 256.0f), 0.08f, 0.75f);
            float nx = ((float)cx / (float)w) * 2.0f - 1.0f;
            float ny = ((float)cy / (float)h) * 2.0f - 1.0f;

            int shapeIdx = (int)(HashRndId(seedIdx, 101, config.seed) * numAvailable) % numAvailable;
            FalloffType shape = availableShapes[shapeIdx];

            float starArms = (float)(minArms + (int)(HashRndId(seedIdx, 102, config.seed) * armsRange) % armsRange);
            float starAmp = std::clamp(HARDCODED_STARFISH_AMP * (0.8f + HashRndId(seedIdx, 103, config.seed) * 0.4f), 0.05f, 0.60f);
            float diamondAngle = HashRndId(seedIdx, 104, config.seed) * 90.0f;
            float diamondAspect = 0.6f + HashRndId(seedIdx, 105, config.seed) * 1.0f;

            continentalLandmassSeeds.push_back(SeedPoint{ nx, ny, rNorm, shape, starArms, starAmp, diamondAngle, diamondAspect });
            seedIdx++;
        }
    }
}

void LandmassGenerator::GenerateContinentalHeightmapData() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    heightmap.assign(w * h, 0.0f);

    FastNoiseLite noise;
    noise.SetSeed(config.seed);
    noise.SetNoiseType(HARDCODED_NOISE_TYPE);
    noise.SetFractalType(config.fractalType);
    noise.SetFractalOctaves(HARDCODED_OCTAVES);
    noise.SetFractalGain(NOISE_GAIN);
    noise.SetFractalLacunarity(NOISE_LACUNARITY);

    float noiseScale = config.frequency * config.details * 10.0f;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int cellId = y * w + x;
            if (IsOceanic(cellPlateOwner[cellId], plateTypes)) {
                heightmap[cellId] = 0.0f;
                continue;
            }

            float u = (float)x / (float)w;
            float v = (float)y / (float)h;
            float nx = u * 2.0f - 1.0f;
            float ny = v * 2.0f - 1.0f;
            float multiSeedMask = continentalLandmassSeeds.empty() ? 1.0f : CalculateMultiSeedMask(nx, ny, continentalLandmassSeeds);
            float sdfClamp = powf(std::clamp(cellSDF[cellId] / std::max(1.0f, config.maxSDFDepth), 0.0f, 1.0f), config.falloffPower);
            float normalizedNoise = (noise.GetNoise(u * noiseScale, v * noiseScale) + 1.0f) * 0.5f;
            heightmap[cellId] = std::clamp(normalizedNoise * multiSeedMask * sdfClamp, 0.0f, 1.0f);
        }
    }
}

Image LandmassGenerator::GenerateRawNoiseImage() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    Image img = GenImageColor(w, h, BLACK);

    for (int i = 0; i < w * h; ++i) {
        if (!IsOceanic(cellPlateOwner[i], plateTypes)) {
            unsigned char byteVal = (unsigned char)(heightmap[i] * 255.0f);
            ImageDrawPixel(&img, i % w, i / w, Color{ byteVal, byteVal, byteVal, 255 });
        }
    }
    return img;
}

/* =========================================================================
   Falloff & Standard Image Outputs
   ========================================================================= */

float LandmassGenerator::EvaluateSeedFalloff(float dx, float dy, const SeedPoint& sp) {
    float r = std::max(0.01f, sp.radius);
    float normD = sqrtf(dx * dx + dy * dy) / r;

    if (sp.shape == FalloffType::Diamond) {
        float rad = sp.diamondAngle * (PI / 180.0f);
        float rx = dx * cosf(rad) - dy * sinf(rad);
        float ry = dx * sinf(rad) + dy * cosf(rad);
        float aspect = std::max(0.1f, sp.diamondAspect);
        float q = std::clamp(HARDCODED_DIAMOND_PINCH, 0.4f, 2.0f);
        normD = powf(powf(fabsf(rx) / (r * aspect), q) + powf(fabsf(ry) / (r / aspect), q), 1.0f / q);
    } else if (sp.shape == FalloffType::Starfish) {
        float angle = atan2f(dy, dx);
        float normRadiusRatio = std::clamp(r / std::max(0.01f, config.seedMaxRadius), 0.1f, 1.0f);
        float effectiveAmp = sp.starAmp * powf(normRadiusRatio, HARDCODED_STARFISH_DAMPENING);
        normD = (sqrtf(dx * dx + dy * dy) * (1.0f + effectiveAmp * sinf(sp.starArms * angle))) / r;
    }

    return powf(std::max(0.0f, 1.0f - normD), config.falloffPower);
}

std::vector<SeedPoint> LandmassGenerator::GenerateSeedPoints() {
    return continentalLandmassSeeds;
}

float LandmassGenerator::CalculateMultiSeedMask(float nx, float ny, const std::vector<SeedPoint>& seeds) {
    if (seeds.empty()) return 1.0f;
    float totalPotential = 0.0f;
    for (const auto& sp : seeds) {
        totalPotential += EvaluateSeedFalloff(nx - sp.x, ny - sp.y, sp);
    }
    return std::clamp(totalPotential, 0.0f, 1.0f);
}

void LandmassGenerator::GenerateLandmassData() {
    GenerateContinentalHeightmapData();
}

Image LandmassGenerator::GenerateCoastlineImage() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    Image img = GenImageColor(w, h, BLACK);

    Color oceanColor     = Color{ 21, 101, 192, 255 };
    Color landColor      = Color{ 46, 125, 50, 255 };
    Color coastlineColor = Color{ 255, 215, 0, 255 };

    for (int i = 0; i < w * h; ++i) {
        float val = heightmap[i];
        Color c;
        if (fabsf(val - config.waterLevel) <= config.coastLineWidth) c = coastlineColor;
        else if (val > config.waterLevel) c = landColor;
        else c = oceanColor;
        ImageDrawPixel(&img, i % w, i / w, c);
    }
    return img;
}

Image LandmassGenerator::GenerateHeightmapImage() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    Image img = GenImageColor(w, h, BLACK);
    Color contourColor = Color{ 0, 225, 255, 255 };

    for (int i = 0; i < w * h; ++i) {
        float val = heightmap[i];
        Color c;
        if (fabsf(val - config.waterLevel) <= config.coastLineWidth) {
            c = contourColor;
        } else {
            unsigned char byteVal = (unsigned char)(std::clamp(val, 0.0f, 1.0f) * 255.0f);
            c = Color{ byteVal, byteVal, byteVal, 255 };
        }
        ImageDrawPixel(&img, i % w, i / w, c);
    }
    return img;
}

Image LandmassGenerator::GenerateGradientMapImage() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    Image img = GenImageColor(w, h, BLACK);

    if (config.sdfBiasMode == SDFBiasMode::MapGradient) {
        for (int y = 0; y < h; ++y) {
            float v = (float)y / (float)h;
            float taperV = 1.0f - config.plateTaperStrengthV * (v - 0.5f) * 2.0f;

            for (int x = 0; x < w; ++x) {
                float u = (float)x / (float)w;
                float taperH = 1.0f - config.plateTaperStrengthH * (u - 0.5f) * 2.0f;
                float taper = std::clamp(taperV * taperH, 0.0f, 2.0f);

                unsigned char val = (unsigned char)(std::clamp(taper * 0.5f, 0.0f, 1.0f) * 255.0f);
                ImageDrawPixel(&img, x, y, Color{ val, val, val, 255 });
            }
        }
    } else if (config.sdfBiasMode == SDFBiasMode::LowFreqNoise) {
        FastNoiseLite lowNoise;
        lowNoise.SetSeed(config.seed + 999);
        lowNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

        float noiseScale = config.lowFreqNoiseFrequency * 2.0f;

        for (int y = 0; y < h; ++y) {
            float v = (float)y / (float)h;
            for (int x = 0; x < w; ++x) {
                float u = (float)x / (float)w;
                float nVal = lowNoise.GetNoise(u * noiseScale * 10.0f, v * noiseScale * 10.0f);
                float bias = std::clamp(1.0f + config.lowFreqNoiseStrength * nVal, 0.0f, 2.0f);

                unsigned char val = (unsigned char)(std::clamp(bias * 0.5f, 0.0f, 1.0f) * 255.0f);
                ImageDrawPixel(&img, x, y, Color{ val, val, val, 255 });
            }
        }
    }
    return img;
}
