#pragma once
#include "Application.hpp"
#include "CustomCamera.hpp"
#include "raylib/raylib.h"

class WorldBuilder : public Application {
private:
    CustomCamera* customCamera;
    Shader wireframeShader;
    Model testModel;

    int lineColorLoc;
    int fillColorLoc;
    int thicknessLoc;

    float lineColArr[4];
    float fillColArr[4];
    float thickness;

public:
    WorldBuilder();

    void Init() override;
    void Update(float deltaTime) override;
    void Draw() override;
    void DrawUI() override;
    void Shutdown() override;
};
