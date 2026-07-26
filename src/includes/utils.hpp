#pragma once
#include "raylib/raylib.h"

enum class PlateType {
    OCEANIC = 0,
    CONTINENTAL = 1
};

struct TectonicPlate {
    int id;
    PlateType type;
    Color color;
};

// Function Declarations
Mesh GenerateIcosphereMesh(int subdivisions, float radius);

Vector3 PolarToCartesian(float lonRad, float latRad, float radius = 1.0f);

void CartesianToPolar(Vector3 pos, float& lonRad, float& latRad);

void PolarToEquirectangularPixel(float lonRad, float latRad, int texWidth, int texHeight, int& px, int& py);

Color GenerateRandomPlateColor(PlateType type, int seed);