#pragma once
#include "utils.hpp"
#include <vector>

enum class BoundaryType {
    NONE = 0,
    CONVERGENT = 1, // Plates colliding (Red: Mountains / Trenches)
    DIVERGENT = 2,  // Plates rifting apart (Cyan: Ocean Ridges / Rift Valleys)
    TRANSFORM = 3   // Plates sliding past each other (Orange: Fault lines)
};

struct BoundaryCellData {
    int cellIndex;
    int plateA;
    int plateB;
    BoundaryType type;
    float compressionRate; // Negative = Convergent, Positive = Divergent
    Vector3 relativeVel;   // Relative 3D velocity vector
};

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

    // Noise Parameters
    int noiseSeed = 1337;
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

    Image textureImage;
    Texture2D texture;

    void Rebuild(int faceRes, float radius, int tWidth = 2048, int tHeight = 1024);
    void RunTectonicPlateAssignment();
    void EvaluateTectonicBoundaryCollisions();
    void RenderPointsToEquirectangularTexture();
    bool ExportTextureImage(const char* filepath);
    void Unload();
};