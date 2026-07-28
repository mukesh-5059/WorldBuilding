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

    std::vector<TectonicPlate> plates;
    std::vector<int> cellPlateOwner; // Maps cell index -> Plate ID

    Image textureImage;
    Texture2D texture;

    void Rebuild(int faceRes, float radius, int tWidth = 2048, int tHeight = 1024);
    void RunTectonicPlateAssignment();
    void RenderPointsToEquirectangularTexture();
    bool ExportTextureImage(const char* filepath);
    void Unload();
};