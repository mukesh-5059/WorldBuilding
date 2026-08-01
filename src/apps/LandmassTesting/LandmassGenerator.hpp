#pragma once
#include "utils.hpp"
#include "fastnoise/FaseNoise.h"
#include "raylib/raylib.h"
#include <vector>

// Hardcoded FastNoise constant parameters
constexpr float NOISE_GAIN = 0.4f;       // Constant gain/persistence
constexpr float NOISE_LACUNARITY = 2.1f; // Constant lacunarity
constexpr FastNoiseLite::NoiseType HARDCODED_NOISE_TYPE = FastNoiseLite::NoiseType_OpenSimplex2;
constexpr int HARDCODED_OCTAVES = 4;

// Hardcoded Shape Refinement constant parameters
constexpr float HARDCODED_STARFISH_ARMS = 10.0f;     // Max arms for randomized starfish modulation
constexpr float HARDCODED_STARFISH_AMP = 0.15f;      // Base amplitude depth for starfish arms
constexpr float HARDCODED_STARFISH_DAMPENING = 2.24f;// Dampens spike amplitude on small starfish seeds
constexpr float HARDCODED_DIAMOND_PINCH = 1.19f;     // Diamond pinch / corner roundness q

enum class SDFBiasMode {
    MapGradient = 0,
    LowFreqNoise = 1
};

enum class FalloffType {
    Radial = 0,
    Diamond = 1,
    Starfish = 2
};

struct SeedPoint {
    float x;
    float y;
    float radius;
    FalloffType shape;
    float starArms;
    float starAmp;
    float diamondAngle;
    float diamondAspect;
};

struct LandmassConfig {
    int mapWidth = 256;
    int mapHeight = 256;
    int seed = 8744; // Default starting seed from user settings

    // Step 2 & 3: Tectonic Plate & SDF Settings
    int numPlates = 16;              // Number of random plate seeds
    float plateSizeVariance = 0.60f; // Random growth step bias variance per plate
    float landRatio = 0.45f;         // Ratio of Continental vs Oceanic plates (0.0 to 1.0)
    float maxSDFDepth = 30.1f;       // Max SDF distance for green gradient normalization
    SDFBiasMode sdfBiasMode = SDFBiasMode::MapGradient; // SDF Bias Mode
    float plateTaperStrengthV = 0.50f;// Vertical map tapering (North-to-South)
    float plateTaperStrengthH = 0.00f;// Horizontal map tapering (West-to-East)
    float lowFreqNoiseStrength = 0.60f;// Low-frequency noise modulation strength
    float lowFreqNoiseFrequency = 1.20f;// Low-frequency noise frequency scale

    // Step 4: Landmass Seed Placement Settings
    float minInteriorSDF = 9.0f;     // Min SDF depth to place landmass seed
    float seedSpacing = 12.2f;       // Poisson-disk spacing between seeds (cells)
    float radiusScale = 1.22f;       // Adaptive radius scale multiplier
    float concentrationPower = 3.46f; // SDF bias exponent for concentrating seeds in continental middle

    bool drawSeedPoints = false;      // Highlight plate seed cells with white cross markers
    bool drawBoundaries = false;      // Highlight continental coastline boundary cells with bright yellow
    bool drawPlateBoundaries = false;// Highlight all inter-plate boundaries with orange/red
    bool drawSDFColors = false;       // Render SDF Green gradient for land & Blue for oceans
    bool drawLandmassSeeds = false;   // Highlight adaptive landmass seed points with magenta circles

    // Coastline & Water Settings
    float waterLevel = 0.050f;
    float coastLineWidth = 0.015f;

    // Multi-Seed Falloff Mask Settings (MultiSeedMetaball & RandomPerSeed)
    int seedCount = 12;
    float seedSpread = 0.84f;
    float seedMinRadius = 0.28f;
    float seedMaxRadius = 0.70f;
    float falloffPower = 1.50f;

    // FastNoise Lite Controls
    FastNoiseLite::FractalType fractalType = FastNoiseLite::FractalType_FBm; // Default FBm
    float frequency = 1.50f;
    float details = 10.88f;
};

class LandmassGenerator {
public:
    LandmassConfig config;
    std::vector<float> heightmap;

    // Step 2: 2D Tectonic Plate Lookup & Priority Queue Dijkstra Expansion
    std::vector<int> cellPlateOwner;   // 1D lookup array mapping cellId -> plateId
    std::vector<float> cellPlateDist;  // Distance array mapping cellId -> min step distance
    std::vector<PlateType> plateTypes; // PlateType (OCEANIC vs CONTINENTAL) per plateId
    std::vector<float> plateGrowthBias;// Random step growth bias per plate
    std::vector<int> plateSeedCells;   // Array of seed cell indices

    // Step 3: Continental Interior Signed Distance Field (SDF)
    std::vector<float> cellSDF;                // 1D lookup array mapping cellId -> distance to nearest ocean plate
    std::vector<bool> cellIsBoundary;          // True for continental cells adjacent to oceanic plates
    std::vector<int> continentalBoundaryCellIds;// List of continental boundary cell indices

    // Inter-Plate Boundary Detection
    std::vector<bool> cellIsPlateBoundary;     // True for cells adjacent to a different plate
    std::vector<int> allPlateBoundaryCellIds;  // List of all inter-plate boundary cells

    // Step 4: Continental Adaptive Landmass Seeds
    std::vector<SeedPoint> continentalLandmassSeeds;

public:
    LandmassGenerator();
    ~LandmassGenerator();

    // Step 2: 2D Plate Assignment via Priority Queue (Dijkstra weighted growth)
    void RunPlateAssignmentFloodFill();
    Image GeneratePlateMapImage();

    // Step 3: Calculate Continental Interior SDF & Detect Inter-Plate Boundaries
    void CalculateBorders();
    Image GeneratePlateSDFMapImage();

    // Step 4: Generate Adaptive Landmass Seeds & Integrated Continental Heightmap
    void GenerateContinentalLandmassSeeds();
    void GenerateContinentalHeightmapData();

    // Unified Pipeline Execution
    void RunFullPipeline();

    // Falloff & Image Outputs
    void GenerateLandmassData();
    std::vector<SeedPoint> GenerateSeedPoints();
    float EvaluateSeedFalloff(float dx, float dy, const SeedPoint& sp);
    float CalculateMultiSeedMask(float nx, float ny, const std::vector<SeedPoint>& seeds);
    Image GenerateCoastlineImage();
    Image GenerateHeightmapImage();
    Image GenerateRawNoiseImage();
    Image GenerateGradientMapImage();
};
