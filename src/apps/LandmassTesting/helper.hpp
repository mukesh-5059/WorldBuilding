#pragma once
#include "utils.hpp"
#include <vector>

// Helper functions for hash generation using core HashCell3D
float HashRndId(int id, int offset, int seed);
float HashRndUV(int cellX, int cellY, int offset, int seed, int mapWidth, int mapHeight);

// Helper for drawing 5-pixel '+' cross markers on images
void DrawCrossMarker(Image* img, int x, int y, Color color, int w, int h);

// Plate type classification helpers
inline bool IsContinental(int owner, const std::vector<PlateType>& plateTypes) {
    return owner >= 0 && owner < (int)plateTypes.size() && plateTypes[owner] == PlateType::CONTINENTAL;
}

inline bool IsOceanic(int owner, const std::vector<PlateType>& plateTypes) {
    return owner < 0 || owner >= (int)plateTypes.size() || plateTypes[owner] == PlateType::OCEANIC;
}

// UI Combo resolution helpers
int GetResIndex(int currentRes);
int GetResFromIndex(int index);
