#include "WorldGenerator.hpp"
#include "ConsoleLog.hpp"
#include <cmath>
#include <vector>
#include <chrono>

Builder::Builder() {
    textureLoaded = false;
    textureImage.data = nullptr;
}

Builder::~Builder() {
    Unload();
}

static int GetCellIdAtPixel(int px, int py, int texWidth, int texHeight, int cubemapFaceRes) {
    float latRad = (PI * 0.5f) - ((float)py + 0.5f) * (PI / (float)texHeight);
    float lonRad = -PI + ((float)px + 0.5f) * (2.0f * PI / (float)texWidth);

    float cosLat = cosf(latRad);
    Vector3 dir = Vector3{ cosLat * cosf(lonRad), sinf(latRad), cosLat * sinf(lonRad) };

    float ax = fabsf(dir.x);
    float ay = fabsf(dir.y);
    float az = fabsf(dir.z);

    int face = 0;
    float uFace = 0.0f, vFace = 0.0f;

    if (ax >= ay && ax >= az) {
        if (dir.x > 0.0f) { face = 0; uFace = -dir.z / dir.x; vFace = dir.y / dir.x; } // +X
        else             { face = 1; uFace =  dir.z / -dir.x; vFace = dir.y / -dir.x; } // -X
    } else if (ay >= ax && ay >= az) {
        if (dir.y > 0.0f) { face = 2; uFace =  dir.x / dir.y; vFace = -dir.z / dir.y; } // +Y
        else             { face = 3; uFace =  dir.x / -dir.y; vFace =  dir.z / -dir.y; } // -Y
    } else {
        if (dir.z > 0.0f) { face = 4; uFace =  dir.x / dir.z; vFace = dir.y / dir.z; } // +Z
        else             { face = 5; uFace = -dir.x / -dir.z; vFace = dir.y / -dir.z; } // -Z
    }

    int i = (int)(((uFace + 1.0f) * 0.5f) * (float)cubemapFaceRes);
    int j = (int)(((vFace + 1.0f) * 0.5f) * (float)cubemapFaceRes);
    if (i < 0) i = 0; if (i >= cubemapFaceRes) i = cubemapFaceRes - 1;
    if (j < 0) j = 0; if (j >= cubemapFaceRes) j = cubemapFaceRes - 1;

    return face * cubemapFaceRes * cubemapFaceRes + i * cubemapFaceRes + j;
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

void Builder::Rebuild(int faceRes, float radius, int tWidth, int tHeight) {
    Unload();

    cubemapFaceRes = faceRes;
    planetRadius = radius;
    texWidth = tWidth;
    texHeight = tHeight;

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
    Color boundaryColor     = Color{ 20, 20, 30, 255 };
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
                bool isBorder = false;
                int borderCellId = cellId;

                int tRadius = 2; // Standard 2px boundary thickness
                for (int dx = -tRadius + 1; dx <= tRadius; ++dx) {
                    for (int dy = -tRadius + 1; dy <= tRadius; ++dy) {
                        if (dx == 0 && dy == 0) continue;
                        int checkPx = (px + dx + texWidth) % texWidth;
                        int checkPy = py + dy;
                        if (checkPy < 0 || checkPy >= texHeight) continue;

                        int checkCell = pixelToCellMap[checkPy * texWidth + checkPx];
                        if (cellPlateOwner[checkCell] != ownerPlate) {
                            isBorder = true;
                            // Pick cell ID of boundary cell that has a valid boundary type
                            borderCellId = (cellPlateOwner[checkCell] < ownerPlate) ? checkCell : cellId;
                            break;
                        }
                    }
                    if (isBorder) break;
                }

                if (isBorder) {
                    if (drawStressBoundaries) {
                        BoundaryType bType = (borderCellId >= 0 && borderCellId < (int)cellBoundaryType.size()) ? cellBoundaryType[borderCellId] : BoundaryType::NONE;
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
                        pixelColor = boundaryColor; // Dark neutral plate boundary
                    }
                }
            }

            pixels[py * texWidth + px] = pixelColor;
        }
    }

    auto tEndTex = std::chrono::high_resolution_clock::now();
    double msTex = std::chrono::duration<double, std::milli>(tEndTex - tStartTex).count();
    ConsoleLog::Get().AddLog(LogLevel::Info, "[Perf] Texture Rendering: %.2f ms", msTex);

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
