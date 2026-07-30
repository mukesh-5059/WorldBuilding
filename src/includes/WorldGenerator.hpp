#pragma once
#include "utils.hpp"
#include <vector>

struct Builder {
    int cubemapFaceRes = 256;

    // Tectonic Controls
    int numPlates = 21;
    bool drawBoundaries = true;
    bool showIcosphereBase = true;
    bool textureLoaded = false;
    int landToWaterRatio = 3;
    float borderJitterStrength = 0.90f;

    int texWidth = 2048;
    int texHeight = 1024;
    float planetRadius = 2.0f;

    float landThreshold = 0.35f;       // Threshold value for land cutoff (0.0 to 1.0)
    float plateSizeVariance = 0.50f;   // Variance in plate expansion speeds (0.0 to 1.0)

    int selectedPlateId = -1;           // Currently selected plate ID (-1 if none)

    std::vector<TectonicPlate> plates;
    std::vector<int> cellPlateOwner; // Maps cell index -> Plate ID
    std::vector<bool> cellIsLand;     // Maps cell index -> True if Land, False if Water
    std::vector<bool> cellIsSeed;     // Maps cell index -> True if Plate Seed Center
    std::vector<float> cellPlateDist; // Maps cell index -> Distance from plate seed center
    std::vector<float> maxPlateDist;   // Maps Plate ID -> Max distance reached for that plate

    Image textureImage;
    Texture2D texture;

    void Rebuild(int faceRes, float radius, int tWidth = 2048, int tHeight = 1024);
    void RunTectonicPlateAssignment();
    void RenderPointsToEquirectangularTexture();
    bool ExportTextureImage(const char* filepath);
    void Unload();
};