#include "LandmassGenerator.hpp"
#include <cmath>
#include <algorithm>
#include <queue>

LandmassGenerator::LandmassGenerator() {
    RunPlateAssignmentFloodFill();
    CalculateContinentalPlateSDF();
    DetectAllPlateBoundaries();
    GenerateContinentalLandmassSeeds();
    GenerateContinentalHeightmapData();
}

LandmassGenerator::~LandmassGenerator() {
}

void LandmassGenerator::GenerateGridCellColors() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    int totalCells = w * h;
    cellColors.resize(totalCells);

    auto HashRndUV = [this](int cellX, int cellY, int offset) {
        int refX = (int)(((float)cellX / (float)config.mapWidth) * 256.0f);
        int refY = (int)(((float)cellY / (float)config.mapHeight) * 256.0f);
        refX = std::clamp(refX, 0, 255);
        refY = std::clamp(refY, 0, 255);
        int id = refY * 256 + refX;
        int n = id * 374761393 + config.seed * 668265263 + offset * 144662241;
        n = (n ^ (n >> 13)) * 1274126177;
        return (float)(n & 0x7fffffff) / (float)0x7fffffff;
    };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int cellId = y * w + x;
            unsigned char r = (unsigned char)(40 + HashRndUV(x, y, 1) * 215.0f);
            unsigned char g = (unsigned char)(40 + HashRndUV(x, y, 2) * 215.0f);
            unsigned char b = (unsigned char)(40 + HashRndUV(x, y, 3) * 215.0f);
            cellColors[cellId] = Color{ r, g, b, 255 };
        }
    }
}

Image LandmassGenerator::GenerateGridCellImage() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    Image img = GenImageColor(w, h, BLACK);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int cellId = y * w + x;
            Color c = cellColors[cellId];
            ImageDrawPixel(&img, x, y, c);
        }
    }
    return img;
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
    plateColors.clear();
    plateTypes.clear();
    plateSeedCells.clear();
    plateGrowthBias.clear();

    auto HashRndId = [this](int id, int offset) {
        int n = id * 374761393 + config.seed * 668265263 + offset * 144662241;
        n = (n ^ (n >> 13)) * 1274126177;
        return (float)(n & 0x7fffffff) / (float)0x7fffffff;
    };

    auto HashRndUV = [this](int cellX, int cellY, int offset) {
        int refX = (int)(((float)cellX / (float)config.mapWidth) * 256.0f);
        int refY = (int)(((float)cellY / (float)config.mapHeight) * 256.0f);
        refX = std::clamp(refX, 0, 255);
        refY = std::clamp(refY, 0, 255);
        int id = refY * 256 + refX;
        int n = id * 374761393 + config.seed * 668265263 + offset * 144662241;
        n = (n ^ (n >> 13)) * 1274126177;
        return (float)(n & 0x7fffffff) / (float)0x7fffffff;
    };

    int numP = std::clamp(config.numPlates, 2, 50);

    // 1. Generate plate colors, growth step bias, and Plate Types (CONTINENTAL vs OCEANIC)
    int continentalCount = 0;
    for (int p = 0; p < numP; ++p) {
        unsigned char r = (unsigned char)(50 + HashRndId(p, 10) * 205.0f);
        unsigned char g = (unsigned char)(50 + HashRndId(p, 20) * 205.0f);
        unsigned char b = (unsigned char)(50 + HashRndId(p, 30) * 205.0f);
        plateColors.push_back(Color{ r, g, b, 255 });

        float hBias = HashRndId(p, 40);
        float gBias = 1.0f + (hBias * 2.0f - 1.0f) * config.plateSizeVariance;
        gBias = std::max(0.15f, gBias);
        plateGrowthBias.push_back(gBias);

        PlateType pType = (HashRndId(p, 60) < config.landRatio) ? PlateType::CONTINENTAL : PlateType::OCEANIC;
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
        int sx = (int)(HashRndId(p, 1) * (float)w);
        int sy = (int)(HashRndId(p, 2) * (float)h);
        sx = std::clamp(sx, 0, w - 1);
        sy = std::clamp(sy, 0, h - 1);

        int seedCell = sy * w + sx;
        if (cellPlateOwner[seedCell] == -1) {
            cellPlateOwner[seedCell] = p;
            cellPlateDist[seedCell] = 0.0f;
            plateSeedCells.push_back(seedCell);
            pq.push(PQElement{ 0.0f, seedCell, p });
        }
    }

    // 3. Scale-Invariant Priority Queue Dijkstra expansion across 4-neighbors
    int dx[4] = { -1, 1, 0, 0 };
    int dy[4] = { 0, 0, -1, 1 };
    float stepScale = 256.0f / (float)w;

    while (!pq.empty()) {
        PQElement top = pq.top();
        pq.pop();

        if (top.dist > cellPlateDist[top.cellId]) {
            continue;
        }

        int cx = top.cellId % w;
        int cy = top.cellId / w;
        float pBias = plateGrowthBias[top.plateId];

        for (int dir = 0; dir < 4; ++dir) {
            int nx = cx + dx[dir];
            int ny = cy + dy[dir];

            if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                int nIdx = ny * w + nx;
                
                float jitter = 0.95f + HashRndUV(nx, ny, 50) * 0.10f;
                float stepCost = (1.0f / pBias) * stepScale * jitter;
                float newDist = top.dist + stepCost;

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

    Color seedColor = Color{ 255, 255, 255, 255 };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int cellId = y * w + x;
            int owner = cellPlateOwner[cellId];
            Color c = (owner >= 0 && owner < (int)plateColors.size()) ? plateColors[owner] : BLACK;
            ImageDrawPixel(&img, x, y, c);
        }
    }

    if (config.drawSeedPoints) {
        for (int seedCell : plateSeedCells) {
            int sx = seedCell % w;
            int sy = seedCell / w;
            ImageDrawPixel(&img, sx, sy, seedColor);
            if (sx > 0) ImageDrawPixel(&img, sx - 1, sy, seedColor);
            if (sx < w - 1) ImageDrawPixel(&img, sx + 1, sy, seedColor);
            if (sy > 0) ImageDrawPixel(&img, sx, sy - 1, seedColor);
            if (sy < h - 1) ImageDrawPixel(&img, sx, sy + 1, seedColor);
        }
    }

    return img;
}

/* =========================================================================
   Step 3: Calculate Scale-Invariant Continental Signed Distance Field (SDF)
   ========================================================================= */

void LandmassGenerator::CalculateContinentalPlateSDF() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    int totalCells = w * h;

    cellSDF.assign(totalCells, 0.0f);
    cellIsBoundary.assign(totalCells, false);
    continentalBoundaryCellIds.clear();

    std::queue<int> q;

    int dx[4] = { -1, 1, 0, 0 };
    int dy[4] = { 0, 0, -1, 1 };
    float stepScale = 256.0f / (float)w;

    // 1. Identify Continental Coastline Boundary Cells
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int cellId = y * w + x;
            int owner = cellPlateOwner[cellId];

            if (owner < 0 || owner >= (int)plateTypes.size() || plateTypes[owner] == PlateType::OCEANIC) {
                cellSDF[cellId] = 0.0f;
                continue;
            }

            bool touchesOcean = false;
            for (int dir = 0; dir < 4; ++dir) {
                int nx = x + dx[dir];
                int ny = y + dy[dir];

                if (nx < 0 || nx >= w || ny < 0 || ny >= h) {
                    touchesOcean = true;
                    break;
                } else {
                    int nIdx = ny * w + nx;
                    int nOwner = cellPlateOwner[nIdx];
                    if (nOwner < 0 || nOwner >= (int)plateTypes.size() || plateTypes[nOwner] == PlateType::OCEANIC) {
                        touchesOcean = true;
                        break;
                    }
                }
            }

            if (touchesOcean) {
                cellIsBoundary[cellId] = true;
                continentalBoundaryCellIds.push_back(cellId);
                cellSDF[cellId] = 0.0f;
                q.push(cellId);
            } else {
                cellSDF[cellId] = 1e9f;
            }
        }
    }

    // 2. Scale-Invariant Multi-Source BFS Distance Propagation Inward
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
                int nOwner = cellPlateOwner[nIdx];

                if (nOwner >= 0 && nOwner < (int)plateTypes.size() && plateTypes[nOwner] == PlateType::CONTINENTAL) {
                    if (cellSDF[nIdx] > currentDist + stepScale) {
                        cellSDF[nIdx] = currentDist + stepScale;
                        q.push(nIdx);
                    }
                }
            }
        }
    }
}

void LandmassGenerator::DetectAllPlateBoundaries() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    int totalCells = w * h;

    cellIsPlateBoundary.assign(totalCells, false);
    allPlateBoundaryCellIds.clear();

    int dx[4] = { -1, 1, 0, 0 };
    int dy[4] = { 0, 0, -1, 1 };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int cellId = y * w + x;
            int owner = cellPlateOwner[cellId];

            bool isBoundary = false;
            for (int dir = 0; dir < 4; ++dir) {
                int nx = x + dx[dir];
                int ny = y + dy[dir];

                if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                    int nIdx = ny * w + nx;
                    if (cellPlateOwner[nIdx] != owner) {
                        isBoundary = true;
                        break;
                    }
                }
            }

            if (isBoundary) {
                cellIsPlateBoundary[cellId] = true;
                allPlateBoundaryCellIds.push_back(cellId);
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
    Color seedWhite          = Color{ 255, 255, 255, 255 }; // Seed Marker White
    Color landmassSeedColor  = Color{ 233, 30, 99, 255 };   // Landmass seed Magenta

    float maxDepth = std::max(1.0f, config.maxSDFDepth);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int cellId = y * w + x;
            int owner = cellPlateOwner[cellId];
            Color c;

            if (owner < 0 || owner >= (int)plateTypes.size() || plateTypes[owner] == PlateType::OCEANIC) {
                int plateIdx = std::max(0, owner);
                Color pColor = (plateIdx < (int)plateColors.size()) ? plateColors[plateIdx] : oceanBaseColor;
                c = Color{
                    (unsigned char)(20 + pColor.r * 0.15f),
                    (unsigned char)(80 + pColor.g * 0.15f),
                    (unsigned char)(180 + pColor.b * 0.20f),
                    255
                };
            } else {
                float sdfVal = cellSDF[cellId];
                float t = std::clamp(sdfVal / maxDepth, 0.0f, 1.0f);

                if (!config.drawSDFColors) {
                    c = Color{ 46, 125, 50, 255 };
                } else {
                    unsigned char r = (unsigned char)((1.0f - t) * 140.0f + t * 15.0f);
                    unsigned char g = (unsigned char)((1.0f - t) * 210.0f + t * 85.0f);
                    unsigned char b = (unsigned char)((1.0f - t) * 90.0f  + t * 30.0f);
                    c = Color{ r, g, b, 255 };
                }
            }
            ImageDrawPixel(&img, x, y, c);
        }
    }

    if (config.drawPlateBoundaries) {
        for (int pbCell : allPlateBoundaryCellIds) {
            int pbx = pbCell % w;
            int pby = pbCell / w;
            ImageDrawPixel(&img, pbx, pby, plateBoundaryColor);
        }
    }

    if (config.drawBoundaries) {
        for (int bCell : continentalBoundaryCellIds) {
            int bx = bCell % w;
            int by = bCell / w;
            ImageDrawPixel(&img, bx, by, boundaryYellow);
        }
    }

    if (config.drawSeedPoints) {
        for (int seedCell : plateSeedCells) {
            int sx = seedCell % w;
            int sy = seedCell / w;
            ImageDrawPixel(&img, sx, sy, seedWhite);
            if (sx > 0) ImageDrawPixel(&img, sx - 1, sy, seedWhite);
            if (sx < w - 1) ImageDrawPixel(&img, sx + 1, sy, seedWhite);
            if (sy > 0) ImageDrawPixel(&img, sx, sy - 1, seedWhite);
            if (sy < h - 1) ImageDrawPixel(&img, sx, sy + 1, seedWhite);
        }
    }

    if (config.drawLandmassSeeds) {
        for (const auto& sp : continentalLandmassSeeds) {
            int lx = (int)(((sp.x + 1.0f) * 0.5f) * (float)w);
            int ly = (int)(((sp.y + 1.0f) * 0.5f) * (float)h);

            if (lx >= 0 && lx < w && ly >= 0 && ly < h) {
                ImageDrawPixel(&img, lx, ly, landmassSeedColor);
                if (lx > 0) ImageDrawPixel(&img, lx - 1, ly, landmassSeedColor);
                if (lx < w - 1) ImageDrawPixel(&img, lx + 1, ly, landmassSeedColor);
                if (ly > 0) ImageDrawPixel(&img, lx, ly - 1, landmassSeedColor);
                if (ly < h - 1) ImageDrawPixel(&img, lx, ly + 1, landmassSeedColor);
            }
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

    auto HashRndId = [this](int id, int offset) {
        int n = id * 374761393 + config.seed * 668265263 + offset * 144662241;
        n = (n ^ (n >> 13)) * 1274126177;
        return (float)(n & 0x7fffffff) / (float)0x7fffffff;
    };

    auto HashRndUV = [this](int cellX, int cellY, int offset) {
        int refX = (int)(((float)cellX / (float)config.mapWidth) * 256.0f);
        int refY = (int)(((float)cellY / (float)config.mapHeight) * 256.0f);
        refX = std::clamp(refX, 0, 255);
        refY = std::clamp(refY, 0, 255);
        int id = refY * 256 + refX;
        int n = id * 374761393 + config.seed * 668265263 + offset * 144662241;
        n = (n ^ (n >> 13)) * 1274126177;
        return (float)(n & 0x7fffffff) / (float)0x7fffffff;
    };

    FalloffType availableShapes[] = {
        FalloffType::Radial,
        FalloffType::Diamond,
        FalloffType::Starfish
    };
    int numAvailable = 3;

    int minArms = 3;
    int maxArms = std::max(3, (int)roundf(HARDCODED_STARFISH_ARMS));
    int armsRange = std::max(1, maxArms - minArms + 1);

    // 1. Gather candidate continental cells with SDF >= minInteriorSDF
    std::vector<int> candidates;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int cellId = y * w + x;
            int owner = cellPlateOwner[cellId];
            if (owner >= 0 && owner < (int)plateTypes.size() && plateTypes[owner] == PlateType::CONTINENTAL) {
                if (cellSDF[cellId] >= config.minInteriorSDF) {
                    candidates.push_back(cellId);
                }
            }
        }
    }

    if (candidates.empty()) return;

    // 2. Sort candidate cells by SDF depth weighted with concentrationPower
    float concPow = std::clamp(config.concentrationPower, 0.1f, 5.0f);

    std::sort(candidates.begin(), candidates.end(), [this, concPow, &HashRndUV](int a, int b) {
        int ax = a % config.mapWidth, ay = a / config.mapWidth;
        int bx = b % config.mapWidth, by = b / config.mapWidth;

        float scoreA = powf(cellSDF[a], concPow) * (0.85f + HashRndUV(ax, ay, 90) * 0.30f);
        float scoreB = powf(cellSDF[b], concPow) * (0.85f + HashRndUV(bx, by, 90) * 0.30f);
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

            float dist = sqrtf((cx - sx) * (cx - sx) + (cy - sy) * (cy - sy));
            if (dist < minSpacing) {
                valid = false;
                break;
            }
        }

        if (valid) {
            float localSDFRef = cellSDF[cellId];
            float rNorm = std::clamp(config.radiusScale * (localSDFRef / 256.0f), 0.08f, 0.75f);

            float nx = ((float)cx / (float)w) * 2.0f - 1.0f;
            float ny = ((float)cy / (float)h) * 2.0f - 1.0f;

            int shapeIdx = (int)(HashRndId(seedIdx, 101) * numAvailable) % numAvailable;
            FalloffType shape = availableShapes[shapeIdx];

            float starArms = (float)(minArms + (int)(HashRndId(seedIdx, 102) * armsRange) % armsRange);
            float starAmp = std::clamp(HARDCODED_STARFISH_AMP * (0.8f + HashRndId(seedIdx, 103) * 0.4f), 0.05f, 0.60f);

            float diamondAngle = HashRndId(seedIdx, 104) * 90.0f;
            float diamondAspect = 0.6f + HashRndId(seedIdx, 105) * 1.0f;

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

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int cellId = y * w + x;
            int owner = cellPlateOwner[cellId];

            if (owner < 0 || owner >= (int)plateTypes.size() || plateTypes[owner] == PlateType::OCEANIC) {
                heightmap[cellId] = 0.0f;
                continue;
            }

            float u = (float)x / (float)w;
            float v = (float)y / (float)h;

            float nx = u * 2.0f - 1.0f;
            float ny = v * 2.0f - 1.0f;

            float multiSeedMask = continentalLandmassSeeds.empty() ? 1.0f : CalculateMultiSeedMask(nx, ny, continentalLandmassSeeds);

            float sdfClamp = std::clamp(cellSDF[cellId] / std::max(1.0f, config.maxSDFDepth), 0.0f, 1.0f);
            sdfClamp = powf(sdfClamp, config.falloffPower);

            float noiseScale = config.frequency * config.details * 10.0f;
            float rawNoise = noise.GetNoise(u * noiseScale, v * noiseScale);
            float normalizedNoise = (rawNoise + 1.0f) * 0.5f;

            heightmap[cellId] = std::clamp(normalizedNoise * multiSeedMask * sdfClamp, 0.0f, 1.0f);
        }
    }
}

Image LandmassGenerator::GenerateRawNoiseImage() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    Image img = GenImageColor(w, h, BLACK);

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
            int owner = cellPlateOwner[cellId];

            if (owner < 0 || owner >= (int)plateTypes.size() || plateTypes[owner] == PlateType::OCEANIC) {
                ImageDrawPixel(&img, x, y, BLACK);
                continue;
            }

            float u = (float)x / (float)w;
            float v = (float)y / (float)h;

            float nx = u * 2.0f - 1.0f;
            float ny = v * 2.0f - 1.0f;

            float multiSeedMask = continentalLandmassSeeds.empty() ? 1.0f : CalculateMultiSeedMask(nx, ny, continentalLandmassSeeds);

            float sdfClamp = std::clamp(cellSDF[cellId] / std::max(1.0f, config.maxSDFDepth), 0.0f, 1.0f);
            sdfClamp = powf(sdfClamp, config.falloffPower);

            float rawNoise = noise.GetNoise(u * noiseScale, v * noiseScale);
            float normalizedNoise = (rawNoise + 1.0f) * 0.5f;

            float finalVal = std::clamp(normalizedNoise * multiSeedMask * sdfClamp, 0.0f, 1.0f);

            unsigned char byteVal = (unsigned char)(finalVal * 255.0f);
            Color c = Color{ byteVal, byteVal, byteVal, 255 };

            ImageDrawPixel(&img, x, y, c);
        }
    }
    return img;
}

/* =========================================================================
   Falloff & Standard Image Outputs
   ========================================================================= */

float LandmassGenerator::EvaluateSeedFalloff(float dx, float dy, const SeedPoint& sp) {
    float r = std::max(0.01f, sp.radius);
    FalloffType shape = sp.shape;

    if (shape == FalloffType::Radial) {
        float normD = sqrtf(dx * dx + dy * dy) / r;
        return powf(std::max(0.0f, 1.0f - normD), config.falloffPower);
    } 
    else if (shape == FalloffType::Diamond) {
        float rad = sp.diamondAngle * (PI / 180.0f);
        float cosA = cosf(rad);
        float sinA = sinf(rad);
        float rx = dx * cosA - dy * sinA;
        float ry = dx * sinA + dy * cosA;

        float aspect = std::max(0.1f, sp.diamondAspect);
        float normX = fabsf(rx) / (r * aspect);
        float normY = fabsf(ry) / (r / aspect);

        float q = std::clamp(HARDCODED_DIAMOND_PINCH, 0.4f, 2.0f);
        float normD = powf(powf(normX, q) + powf(normY, q), 1.0f / q);

        return powf(std::max(0.0f, 1.0f - normD), config.falloffPower);
    } 
    else if (shape == FalloffType::Starfish) {
        float angle = atan2f(dy, dx);
        float normRadiusRatio = std::clamp(r / std::max(0.01f, config.seedMaxRadius), 0.1f, 1.0f);
        float effectiveAmp = sp.starAmp * powf(normRadiusRatio, HARDCODED_STARFISH_DAMPENING);
        float modulation = 1.0f + effectiveAmp * sinf(sp.starArms * angle);
        float dist = sqrtf(dx * dx + dy * dy) * modulation;
        float normD = dist / r;
        return powf(std::max(0.0f, 1.0f - normD), config.falloffPower);
    }

    float normD = sqrtf(dx * dx + dy * dy) / r;
    return powf(std::max(0.0f, 1.0f - normD), config.falloffPower);
}

std::vector<SeedPoint> LandmassGenerator::GenerateSeedPoints() {
    return continentalLandmassSeeds;
}

float LandmassGenerator::CalculateMultiSeedMask(float nx, float ny, const std::vector<SeedPoint>& seeds) {
    if (seeds.empty()) return 1.0f;

    if (config.falloffMode == FalloffMode::SingleCenter) {
        return EvaluateSeedFalloff(nx - seeds[0].x, ny - seeds[0].y, seeds[0]);
    }

    if (config.falloffMode == FalloffMode::MultiSeedNearest) {
        float maxVal = 0.0f;
        for (const auto& sp : seeds) {
            float val = EvaluateSeedFalloff(nx - sp.x, ny - sp.y, sp);
            if (val > maxVal) {
                maxVal = val;
            }
        }
        return maxVal;
    }

    if (config.falloffMode == FalloffMode::MultiSeedMetaball) {
        float totalPotential = 0.0f;
        for (const auto& sp : seeds) {
            float val = EvaluateSeedFalloff(nx - sp.x, ny - sp.y, sp);
            totalPotential += val;
        }
        return std::clamp(totalPotential, 0.0f, 1.0f);
    }

    return 1.0f;
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

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float val = heightmap[y * w + x];
            Color c;

            if (fabsf(val - config.waterLevel) <= config.coastLineWidth) {
                c = coastlineColor;
            } else if (val > config.waterLevel) {
                c = landColor;
            } else {
                c = oceanColor;
            }
            ImageDrawPixel(&img, x, y, c);
        }
    }
    return img;
}

Image LandmassGenerator::GenerateHeightmapImage() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    Image img = GenImageColor(w, h, BLACK);

    Color contourColor = Color{ 0, 225, 255, 255 };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float val = heightmap[y * w + x];
            Color c;

            if (fabsf(val - config.waterLevel) <= config.coastLineWidth) {
                c = contourColor;
            } else {
                unsigned char byteVal = (unsigned char)(std::clamp(val, 0.0f, 1.0f) * 255.0f);
                c = Color{ byteVal, byteVal, byteVal, 255 };
            }
            ImageDrawPixel(&img, x, y, c);
        }
    }
    return img;
}
