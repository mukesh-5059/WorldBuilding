#pragma once
#include "utils.hpp"
#include <vector>

class Builder {
public:
    Builder();
    ~Builder();

    int cubemapFaceRes = 256;

    // Tectonic Controls
    int numPlates = 21;
    bool drawBoundaries = true;
    bool drawStressBoundaries = true;
    bool showIcosphereBase = true;
    bool textureLoaded = false;
    int landToWaterRatio = 3;
    float borderJitterStrength = 0.90f;

    int texWidth = 2048;
    int texHeight = 1024;
    float planetRadius = 2.0f;

    float landThreshold = 0.35f;       // Threshold value for land cutoff (0.0 to 1.0)
    float seaLevel = 0.35f;            // Sea level threshold for land/water cutoff (0.0 to 1.0)
    float plateSizeVariance = 0.50f;   // Variance in plate expansion speeds (0.0 to 1.0)

    // Seed & Control Parameters
    int worldSeed = 42;                // Common seed (0 to 1000) for both spatial HashCell3D & FastNoiseLite

    // Noise Parameters
    int noiseSeed = 42;
    int noiseType = 0;           // 0: OpenSimplex2, 1: OpenSimplex2S, 2: Cellular, 3: Perlin, 4: ValueCubic, 5: Value
    int noiseFractalType = 1;    // 0: None, 1: FBm, 2: Ridged, 3: PingPong
    float noiseFrequency = 2.5f;
    int noiseOctaves = 4;
    float noiseLacunarity = 2.0f;
    float noiseGain = 0.5f;
    float noiseStrength = 0.5f;

    int selectedPlateId = -1;           // Currently selected plate ID (-1 if none)

    std::vector<TectonicPlate> plates;
    std::vector<int> cellPlateOwner;  // Maps cell index -> Plate ID
    std::vector<bool> cellIsLand;      // Maps cell index -> True if Land, False if Water
    std::vector<bool> cellIsSeed;      // Maps cell index -> True if Plate Seed Center
    std::vector<float> cellPlateDist;  // Maps cell index -> Distance from plate seed center
    std::vector<float> maxPlateDist;   // Maps Plate ID -> Max distance reached for that plate

    std::vector<BoundaryType> cellBoundaryType;
    std::vector<float> cellCompressionRate;
    std::vector<BoundaryCellData> boundaryCells;

    std::vector<int> pixelToCellMap;
    int cachedTexWidth = 0;
    int cachedTexHeight = 0;
    int cachedCubemapFaceRes = 0;

    std::vector<Vector3> cell3DVectorMap;
    int cachedCellVectorFaceRes = 0;

    Image textureImage;
    Texture2D texture;

    void PrecomputePixelToCellMap();
    void PrecomputeCell3DVectors();
    Vector3 GetCell3DVectorCached(int cellIndex) const;
    void Rebuild(int faceRes, float radius, int tWidth = 2048, int tHeight = 1024);
    void RunTectonicPlateAssignment();
    void EvaluateTectonicBoundaryCollisions();
    void RenderPointsToEquirectangularTexture();
    bool ExportTextureImage(const char* filepath);
    void Unload();
};