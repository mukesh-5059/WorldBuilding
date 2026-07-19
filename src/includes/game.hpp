#pragma once
#include "Application.hpp"
#include "CustomCamera.hpp"
#include "raylib/raylib.h"

class WorldBuilder : public Application {
private:
    CustomCamera* customCamera;
    Shader wireframeShader;

    int lineColorLoc;
    int fillColorLoc;
    int thicknessLoc;

    float lineColArr[4];
    float fillColArr[4];
    float thickness;

    Mesh icosphereMesh;
    Model icosphereModel;
    bool modelGenerated;
    int subdivisions;
    int lastSubdivisions;
    float radius;
    float lastRadius;

public:
    WorldBuilder();

    void Init() override;
    void Update(float deltaTime) override;
    void Draw() override;
    void DrawUI() override;
    void Shutdown() override;
};
