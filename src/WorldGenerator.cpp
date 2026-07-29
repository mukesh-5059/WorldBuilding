#include "includes/WorldGenerator.hpp"
//#include "fastnoise/FaseNoise.h"
#include <cmath>
#include <vector>


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


void Builder::Rebuild(int faceRes, float radius, int tWidth, int tHeight) {
    Unload();

    cubemapFaceRes = faceRes;
    planetRadius = radius;
    texWidth = tWidth;
    texHeight = tHeight;

    RunTectonicPlateAssignment();

    textureImage = GenImageColor(texWidth, texHeight, WHITE);
    RenderPointsToEquirectangularTexture();
    texture = LoadTextureFromImage(textureImage);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    textureLoaded = true;
}

void Builder::RenderPointsToEquirectangularTexture() {
    if (!textureImage.data || cellPlateOwner.empty()) return;

    Color* pixels = (Color*)textureImage.data;
    Color boundaryColor     = Color{ 20, 20, 30, 255 };
    Color defaultLandColor  = Color{ 46, 139, 87, 255 };   // Land Green
    Color defaultOceanColor = Color{ 25, 75, 150, 255 };   // Deep Ocean Blue
    Color seedColor         = Color{ 235, 30, 30, 255 };   // Red Seed Marker

    for (int py = 0; py < texHeight; ++py) {
        for (int px = 0; px < texWidth; ++px) {
            int cellId = GetCellIdAtPixel(px, py, texWidth, texHeight, cubemapFaceRes);
            int ownerPlate = (cellId >= 0 && cellId < (int)cellPlateOwner.size()) ? cellPlateOwner[cellId] : 0;
            bool isLand = (cellId >= 0 && cellId < (int)cellIsLand.size()) ? cellIsLand[cellId] : false;
            bool isSeed = (cellId >= 0 && cellId < (int)cellIsSeed.size()) ? cellIsSeed[cellId] : false;

            Color pixelColor = isLand ? defaultLandColor : defaultOceanColor;

            if (isSeed) {
                pixelColor = seedColor;
            } else if (drawBoundaries) {
                // Accurate 1-pixel boundary sampling for tectonic plates
                int rightCell = GetCellIdAtPixel((px + 1) % texWidth, py, texWidth, texHeight, cubemapFaceRes);
                int downCell  = GetCellIdAtPixel(px, (py + 1) % texHeight, texWidth, texHeight, cubemapFaceRes);
                if (cellPlateOwner[rightCell] != ownerPlate || cellPlateOwner[downCell] != ownerPlate) {
                    pixelColor = boundaryColor;
                }
            }

            pixels[py * texWidth + px] = pixelColor;
        }
    }

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
