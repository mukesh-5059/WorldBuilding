#pragma once
#include "Application.hpp"
#include "LandmassGenerator.hpp"
#include "raylib/raylib.h"

class LandmassTesting : public Application {
private:
    LandmassGenerator generator;

    Texture2D coastlineTexture = { 0 };
    Texture2D heightmapTexture = { 0 };
    Texture2D rawNoiseTexture = { 0 };
    Texture2D gradientTexture = { 0 };

    int coastlineViewportId = -1;
    int heightmapViewportId = -1;
    int rawNoiseViewportId = -1;
    int gradientViewportId = -1;

    bool autoReloadOnSliderChange = true;

public:
    LandmassTesting();
    ~LandmassTesting() override;

    void Init() override;
    void Update(float deltaTime) override;
    void SceneDraw() override {} // Pure 2D mode uses Texture Viewports
    void DrawUI() override;
    void Shutdown() override;

private:
    Texture2D ReloadCoastlineCallback();
    Texture2D ReloadHeightmapCallback();
    Texture2D ReloadRawNoiseCallback();
    Texture2D ReloadGradientCallback();
    void TriggerViewportReloads();
};
