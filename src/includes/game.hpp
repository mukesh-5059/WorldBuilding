#pragma once
#include "Application.hpp"
#include "CustomCamera.hpp"
#include "WorldGenerator.hpp"
#include "raylib/raylib.h"

class WorldBuilder : public Application {
private:
    CustomCamera* customCamera;
    Shader wireframeShader;
    Shader planetShader;

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

    Builder plotter;
    int uiLonResIndex;

public:
    WorldBuilder();

    void Init() override;
    void Update(float deltaTime) override;
    void SceneDraw() override;
    void DrawUI() override;
    void Shutdown() override;

private:
    void RebuildPlotter();
};
