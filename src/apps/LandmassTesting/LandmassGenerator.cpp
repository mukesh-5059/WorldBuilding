#include "LandmassGenerator.hpp"
#include <cmath>
#include <algorithm>

LandmassGenerator::LandmassGenerator() {
    GenerateLandmassData();
}

LandmassGenerator::~LandmassGenerator() {
}

float LandmassGenerator::EvaluateSeedFalloff(float dx, float dy, const SeedPoint& sp) {
    float r = std::max(0.01f, sp.radius);
    FalloffType shape = sp.shape;

    if (shape == FalloffType::Radial) {
        float normD = sqrtf(dx * dx + dy * dy) / r;
        return powf(std::max(0.0f, 1.0f - normD), config.falloffPower);
    } 
    else if (shape == FalloffType::Diamond) {
        // 1. Rotation by per-seed angle theta
        float rad = sp.diamondAngle * (PI / 180.0f);
        float cosA = cosf(rad);
        float sinA = sinf(rad);
        float rx = dx * cosA - dy * sinA;
        float ry = dx * sinA + dy * cosA;

        // 2. Per-seed Aspect Ratio stretching (elongation / scale)
        float aspect = std::max(0.1f, sp.diamondAspect);
        float normX = fabsf(rx) / (r * aspect);
        float normY = fabsf(ry) / (r / aspect);

        // 3. Hardcoded Pinch L_q norm exponent q = 1.19
        float q = std::clamp(HARDCODED_DIAMOND_PINCH, 0.4f, 2.0f);
        float normD = powf(powf(normX, q) + powf(normY, q), 1.0f / q);

        return powf(std::max(0.0f, 1.0f - normD), config.falloffPower);
    } 
    else if (shape == FalloffType::Starfish) {
        float angle = atan2f(dy, dx);
        // Radius-dependent arm dampening (hardcoded dampening exponent = 2.24)
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
    std::vector<SeedPoint> points;

    auto HashRnd = [this](int id, int offset) {
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

    if (config.falloffMode == FalloffMode::SingleCenter || config.seedCount <= 1) {
        FalloffType shape = config.falloffType;
        if (shape == FalloffType::RandomPerSeed) {
            shape = availableShapes[(int)(HashRnd(0, 4) * numAvailable) % numAvailable];
        }
        float starArms = (float)(minArms + (int)(HashRnd(0, 5) * armsRange) % armsRange);
        float diamondAngle = HashRnd(0, 7) * 90.0f;
        float diamondAspect = 0.6f + HashRnd(0, 8) * 1.0f; // 0.6 to 1.6 aspect ratio
        points.push_back(SeedPoint{ 0.0f, 0.0f, 1.0f, shape, starArms, HARDCODED_STARFISH_AMP, diamondAngle, diamondAspect });
        return points;
    }

    for (int i = 0; i < config.seedCount; ++i) {
        float angle = HashRnd(i, 1) * 2.0f * PI;
        float dist = sqrtf(HashRnd(i, 2)) * config.seedSpread;
        float sx = cosf(angle) * dist;
        float sy = sinf(angle) * dist;
        float r = config.seedMinRadius + HashRnd(i, 3) * std::max(0.01f, config.seedMaxRadius - config.seedMinRadius);

        FalloffType shape = config.falloffType;
        if (shape == FalloffType::RandomPerSeed) {
            int shapeIdx = (int)(HashRnd(i, 4) * numAvailable) % numAvailable;
            shape = availableShapes[shapeIdx];
        }

        // Randomize number of arms per starfish seed in range [3, HARDCODED_STARFISH_ARMS]
        float starArms = (float)(minArms + (int)(HashRnd(i, 5) * armsRange) % armsRange);
        float starAmp = std::clamp(HARDCODED_STARFISH_AMP * (0.8f + HashRnd(i, 6) * 0.4f), 0.05f, 0.60f);

        // Randomize diamond rotation angle [0..90 deg] and aspect ratio [0.6..1.6] per seed center
        float diamondAngle = HashRnd(i, 7) * 90.0f;
        float diamondAspect = 0.6f + HashRnd(i, 8) * 1.0f;

        points.push_back(SeedPoint{ sx, sy, r, shape, starArms, starAmp, diamondAngle, diamondAspect });
    }
    return points;
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
    int w = config.mapWidth;
    int h = config.mapHeight;
    heightmap.assign(w * h, 0.0f);

    std::vector<SeedPoint> seeds = GenerateSeedPoints();

    FastNoiseLite noise;
    noise.SetSeed(config.seed);
    noise.SetNoiseType(HARDCODED_NOISE_TYPE);     // Hardcoded OpenSimplex2
    noise.SetFractalType(HARDCODED_FRACTAL_TYPE); // Hardcoded Ridged
    noise.SetFractalOctaves(HARDCODED_OCTAVES);   // Hardcoded Octaves = 4
    noise.SetFractalGain(NOISE_GAIN);             // Hardcoded Gain = 0.4f
    noise.SetFractalLacunarity(NOISE_LACUNARITY); // Hardcoded Lacunarity = 2.1f

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            // Normalized coordinates [0..1] for grid-resolution invariance
            float u = (float)x / (float)w;
            float v = (float)y / (float)h;

            // Normalized centered coordinates [-1..1] for falloff mask
            float nx = u * 2.0f - 1.0f;
            float ny = v * 2.0f - 1.0f;

            float mask = CalculateMultiSeedMask(nx, ny, seeds);

            // Frequency & Details multiplier relative to normalized UV coordinates (resolution-independent!)
            float noiseScale = config.frequency * config.details * 10.0f;
            float rawNoise = noise.GetNoise(u * noiseScale, v * noiseScale);
            float normalizedNoise = (rawNoise + 1.0f) * 0.5f;

            heightmap[y * w + x] = std::clamp(normalizedNoise * mask, 0.0f, 1.0f);
        }
    }
}

Image LandmassGenerator::GenerateCoastlineImage() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    Image img = GenImageColor(w, h, BLACK);

    Color oceanColor     = Color{ 21, 101, 192, 255 };  // Deep Blue
    Color landColor      = Color{ 46, 125, 50, 255 };   // Emerald Green
    Color coastlineColor = Color{ 255, 215, 0, 255 };   // Bright Gold Outline

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float val = heightmap[y * w + x];
            Color c;

            if (fabsf(val - config.waterLevel) <= config.coastLineWidth) {
                c = coastlineColor; // Highlight coastline boundary edge
            } else if (val > config.waterLevel) {
                c = landColor;      // Landmass
            } else {
                c = oceanColor;     // Ocean
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

    Color contourColor = Color{ 0, 225, 255, 255 }; // Bright Cyan Coastline Overlay

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
