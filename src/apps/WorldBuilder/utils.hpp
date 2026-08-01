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
    float growthBias;  // Growth step weight (lower = larger plate, higher = smaller plate)
    Vector3 eulerPole; // 3D Euler Rotation Axis (Unit Vector)
    float angularSpeed;// Angular rotation speed magnitude
};

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

struct PQElement {
    float dist;
    int cellIndex;
    int plateId;

    bool operator>(const PQElement& other) const {
        return dist > other.dist;
    }
};

Mesh GenerateIcosphereMesh(int subdivisions, float radius);

Vector3 PolarToCartesian(float lonRad, float latRad, float radius = 1.0f);

void CartesianToPolar(Vector3 pos, float& lonRad, float& latRad);

void PolarToEquirectangularPixel(float lonRad, float latRad, int texWidth, int texHeight, int& px, int& py);

Color GetPlateColor(PlateType type);

float HashCell3D(Vector3 p, int seed = 0);

Vector3 GetCell3DVector(int cellIndex, int N);

int GetCellIdFrom3DVector(Vector3 dir, int cubemapFaceRes);

int GetCubemapNeighborCellIndex(int face, int i, int j, int dir, int N);

int GetCellIdAtPixel(int px, int py, int texWidth, int texHeight, int cubemapFaceRes);