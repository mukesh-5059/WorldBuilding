#include "helper.hpp"
#include <algorithm>

float HashRndId(int id, int offset, int seed) {
    return HashCell3D(Vector3{ (float)id, (float)seed, (float)offset });
}

float HashRndUV(int cellX, int cellY, int offset, int seed, int mapWidth, int mapHeight) {
    int refX = std::clamp((int)(((float)cellX / (float)mapWidth) * 256.0f), 0, 255);
    int refY = std::clamp((int)(((float)cellY / (float)mapHeight) * 256.0f), 0, 255);
    return HashRndId(refY * 256 + refX, offset, seed);
}

void DrawCrossMarker(Image* img, int x, int y, Color color, int w, int h) {
    if (x < 0 || x >= w || y < 0 || y >= h) return;
    ImageDrawPixel(img, x, y, color);
    if (x > 0) ImageDrawPixel(img, x - 1, y, color);
    if (x < w - 1) ImageDrawPixel(img, x + 1, y, color);
    if (y > 0) ImageDrawPixel(img, x, y - 1, color);
    if (y < h - 1) ImageDrawPixel(img, x, y + 1, color);
}

int GetResIndex(int currentRes) {
    static const int resValues[] = { 32, 64, 128, 256, 512, 1024 };
    for (int i = 0; i < 6; ++i) {
        if (resValues[i] == currentRes) return i;
    }
    return 3; // Default 256x256
}

int GetResFromIndex(int index) {
    static const int resValues[] = { 32, 64, 128, 256, 512, 1024 };
    if (index >= 0 && index < 6) return resValues[index];
    return 256;
}
