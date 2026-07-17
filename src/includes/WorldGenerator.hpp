#pragma once
#include "raylib/raylib.h"
#include <vector>
#include <atomic>
#include <string>

struct WorldTile {
    int id;
    Vector3 center;
    Vector3 corners[3];
    int neighbors[3];
    int plateId;
};

struct World {
    std::vector<WorldTile> tiles;
    std::vector<Color> plateColors;
    Mesh mesh;
    Model model;
    float radius;
    int subdivisions;
    int numPlates;
    bool generated;
};

World GenerateWorld(int subdivisions, float radius, int numPlates);
void RebuildWorldModel(World& world, Shader shader);
void UnloadWorld(World& world);
void ExportWorldMapAsync(const std::vector<WorldTile>& tiles, const std::vector<Color>& plateColors, const std::string& filepath, int width, int height, std::atomic<float>& progress, std::atomic<bool>& running);
