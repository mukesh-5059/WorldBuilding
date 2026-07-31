#pragma once
#include "Application.hpp"
#include "fastnoise/FaseNoise.h"
#include "raylib/raylib.h"
#include <vector>

// Hardcoded FastNoise constant parameters
constexpr float NOISE_GAIN = 0.4f;       // Constant gain/persistence
constexpr float NOISE_LACUNARITY = 2.1f; // Constant lacunarity
constexpr FastNoiseLite::NoiseType HARDCODED_NOISE_TYPE = FastNoiseLite::NoiseType_OpenSimplex2;
constexpr FastNoiseLite::FractalType HARDCODED_FRACTAL_TYPE = FastNoiseLite::FractalType_Ridged;
constexpr int HARDCODED_OCTAVES = 4;

// Hardcoded Shape Refinement constant parameters
constexpr float HARDCODED_STARFISH_ARMS = 10.0f;     // Max arms for randomized starfish modulation
constexpr float HARDCODED_STARFISH_AMP = 0.15f;      // Base amplitude depth for starfish arms
constexpr float HARDCODED_STARFISH_DAMPENING = 2.24f;// Dampens spike amplitude on small starfish seeds
constexpr float HARDCODED_DIAMOND_PINCH = 1.19f;     // Diamond pinch / corner roundness q

enum class FalloffType {
    Radial = 0,
    Diamond = 1,
    Starfish = 2,
    RandomPerSeed = 3 // Assigns a random falloff shape (Radial, Diamond, Starfish) per seed center!
};

enum class FalloffMode {
    SingleCenter = 0,
    MultiSeedNearest = 1,
    MultiSeedMetaball = 2
};

struct SeedPoint {
    float x;
    float y;
    float radius;
    FalloffType shape;
    float starArms;
    float starAmp;
    float diamondAngle;  // Randomized per-seed rotation angle [0..90 deg]
    float diamondAspect; // Randomized per-seed aspect ratio [0.6..1.6]
};

struct IslandConfig {
    int mapWidth = 256;
    int mapHeight = 256;
    int seed = 328; // User default starting seed

    // Coastline & Water Settings
    float waterLevel = 0.146f;
    float coastLineWidth = 0.015f;

    // Multi-Seed Falloff Mask Settings
    FalloffMode falloffMode = FalloffMode::MultiSeedMetaball;
    FalloffType falloffType = FalloffType::RandomPerSeed; // Random per-seed by default!
    int seedCount = 12;            // Number of island seed centers
    float seedSpread = 0.84f;      // How spread out seed centers are across the map
    float seedMinRadius = 0.28f;   // Min radius of influence per seed
    float seedMaxRadius = 0.70f;   // Max radius of influence per seed
    float falloffPower = 1.41f;     // Falloff curve exponent

    // FastNoise Lite Controls (Grid-Resolution Invariant)
    float frequency = 1.50f;        // Normalized noise frequency (scale-invariant with grid resolution)
    float details = 10.88f;         // Details multiplier for fine coastline noise scale
};

class IslandTesting : public Application {
private:
    IslandConfig config;
    std::vector<float> heightmap;

    Texture2D coastlineTexture = { 0 };
    Texture2D heightmapTexture = { 0 };

    int coastlineViewportId = -1;
    int heightmapViewportId = -1;

    bool autoReloadOnSliderChange = true;

public:
    IslandTesting();
    ~IslandTesting() override;

    void Init() override;
    void Update(float deltaTime);
    void SceneDraw() override {} // Pure 2D mode uses Texture Viewports
    void DrawUI() override;
    void Shutdown() override;

private:
    void GenerateIslandData();
    std::vector<SeedPoint> GenerateSeedPoints();
    float EvaluateSeedFalloff(float dx, float dy, const SeedPoint& sp);
    float CalculateMultiSeedMask(float nx, float ny, const std::vector<SeedPoint>& seeds);
    Image GenerateCoastlineImage();
    Image GenerateHeightmapImage();
    Texture2D ReloadCoastlineCallback();
    Texture2D ReloadHeightmapCallback();
    void TriggerViewportReloads();
};
