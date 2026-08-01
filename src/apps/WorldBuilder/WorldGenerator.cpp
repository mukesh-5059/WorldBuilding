#include "WorldGenerator.hpp"
#include "ConsoleLog.hpp"
#include <vector>
#include <chrono>

Builder::Builder() {
    textureLoaded = false;
    textureImage.data = nullptr;
}

Builder::~Builder() {
    Unload();
}

void Builder::PrecomputePixelToCellMap() {
    if ((int)pixelToCellMap.size() == texWidth * texHeight &&
        cachedTexWidth == texWidth &&
        cachedTexHeight == texHeight &&
        cachedCubemapFaceRes == cubemapFaceRes) {
        return; // Cache hit
    }

    pixelToCellMap.resize(texWidth * texHeight);
    cachedTexWidth = texWidth;
    cachedTexHeight = texHeight;
    cachedCubemapFaceRes = cubemapFaceRes;

    for (int py = 0; py < texHeight; ++py) {
        for (int px = 0; px < texWidth; ++px) {
            pixelToCellMap[py * texWidth + px] = GetCellIdAtPixel(px, py, texWidth, texHeight, cubemapFaceRes);
        }
    }
}

void Builder::PrecomputeCell3DVectors() {
    int totalCells = 6 * cubemapFaceRes * cubemapFaceRes;
    if ((int)cell3DVectorMap.size() == totalCells && cachedCellVectorFaceRes == cubemapFaceRes) {
        return; // Cache hit
    }

    cell3DVectorMap.resize(totalCells);
    cachedCellVectorFaceRes = cubemapFaceRes;

    for (int c = 0; c < totalCells; ++c) {
        cell3DVectorMap[c] = GetCell3DVector(c, cubemapFaceRes);
    }
}

Vector3 Builder::GetCell3DVectorCached(int cellIndex) const {
    if (cellIndex >= 0 && cellIndex < (int)cell3DVectorMap.size()) {
        return cell3DVectorMap[cellIndex];
    }
    return GetCell3DVector(cellIndex, cubemapFaceRes);
}

void Builder::Rebuild(int faceRes, float radius, int tWidth, int tHeight) {
    Unload();

    cubemapFaceRes = faceRes;
    planetRadius = radius;
    texWidth = tWidth;
    texHeight = tHeight;

    PrecomputeCell3DVectors();

    RunTectonicPlateAssignment();
    EvaluateTectonicBoundaryCollisions();

    textureImage = GenImageColor(texWidth, texHeight, WHITE);
    RenderPointsToEquirectangularTexture();
    texture = LoadTextureFromImage(textureImage);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    textureLoaded = true;
}

void Builder::RenderPointsToEquirectangularTexture() {
    if (!textureImage.data || cellPlateOwner.empty()) return;

    auto tStartTex = std::chrono::high_resolution_clock::now();

    PrecomputePixelToCellMap();

    Color* pixels = (Color*)textureImage.data;
    Color boundaryColor     = Color{ 20, 20, 30, 255 };    // Dark neutral plate boundary
    Color defaultLandColor  = Color{ 46, 139, 87, 255 };   // Land Green
    Color defaultOceanColor = Color{ 25, 75, 150, 255 };   // Deep Ocean Blue
    Color seedColor         = Color{ 235, 30, 30, 255 };   // Red Seed Marker

    for (int py = 0; py < texHeight; ++py) {
        for (int px = 0; px < texWidth; ++px) {
            int cellId = pixelToCellMap[py * texWidth + px];
            int ownerPlate = (cellId >= 0 && cellId < (int)cellPlateOwner.size()) ? cellPlateOwner[cellId] : 0;
            bool isLand = (cellId >= 0 && cellId < (int)cellIsLand.size()) ? cellIsLand[cellId] : false;
            bool isSeed = (cellId >= 0 && cellId < (int)cellIsSeed.size()) ? cellIsSeed[cellId] : false;

            Color pixelColor = isLand ? defaultLandColor : defaultOceanColor;

            if (isSeed) {
                pixelColor = seedColor;
            } else if (drawStressBoundaries || drawBoundaries) {
                BoundaryType bType = (cellId >= 0 && cellId < (int)cellBoundaryType.size()) ? cellBoundaryType[cellId] : BoundaryType::NONE;
                if (bType != BoundaryType::NONE) {
                    if (drawStressBoundaries) {
                        if (bType == BoundaryType::CONVERGENT) {
                            pixelColor = Color{ 235, 40, 40, 255 };  // Red (Colliding / Mountains)
                        } else if (bType == BoundaryType::DIVERGENT) {
                            pixelColor = Color{ 40, 200, 235, 255 }; // Cyan (Rifting / Ridges)
                        } else if (bType == BoundaryType::TRANSFORM) {
                            pixelColor = Color{ 240, 160, 40, 255 }; // Orange (Fault lines)
                        } else {
                            pixelColor = boundaryColor;
                        }
                    } else {
                        pixelColor = boundaryColor;
                    }
                }
            }

            pixels[py * texWidth + px] = pixelColor;
        }
    }

    auto tEndTex = std::chrono::high_resolution_clock::now();
    double msTex = std::chrono::duration<double, std::milli>(tEndTex - tStartTex).count();
    ConsoleLog::Get().AddLog(LogLevel::Performance, "[Perf] Texture Rendering: %.2f ms", msTex);

    if (textureLoaded) {
        UpdateTexture(texture, textureImage.data);
    }
}

bool Builder::ExportTextureImage(const char* filepath) {
    if (!textureLoaded || !textureImage.data) return false;
    return ExportImage(textureImage, filepath);
}

void Builder::Unload() {
    if (textureLoaded) {
        UnloadTexture(texture);
        UnloadImage(textureImage);
        textureLoaded = false;
    }
}
