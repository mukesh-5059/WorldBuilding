#pragma once
#include "Application.hpp"
#include "CustomCamera.hpp"
#include "WorldGenerator.hpp"
#include "raylib/raylib.h"

class WorldBuilder : public Application {
private:
    CustomCamera* customCamera;
    Shader wireframeShader;
    World world;

    int lineColorLoc;
    int fillColorLoc;
    int thicknessLoc;

    float lineColArr[4];
    float fillColArr[4];
    float thickness;

    int subdivisions;
    int lastSubdivisions;
    float radius;
    float lastRadius;
    int numPlates;
    int lastNumPlates;

    std::atomic<bool> exportRunning;
    std::atomic<float> exportProgress;
    int exportWidth;
    int exportHeight;

public:
    WorldBuilder();

    void Init() override;
    void Update(float deltaTime) override;
    void Draw() override;
    void DrawUI() override;
    void Shutdown() override;
};
