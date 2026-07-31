#include "LandmassTesting.hpp"
#include "ConsoleLog.hpp"
#include "imgui/imgui.h"
#include "raylib/raymath.h"
#include "rlimgui/rlImGui.h"
#include <algorithm>

LandmassTesting::LandmassTesting()
    : Application(1280, 720, "2D Tectonic Plate & Continental Landmass Engine") {
    show3DViewportTab = false; // Pure 2D texture viewport mode
}

LandmassTesting::~LandmassTesting() {
}

Texture2D LandmassTesting::ReloadCoastlineCallback() {
    generator.RunPlateAssignmentFloodFill();
    generator.CalculateContinentalPlateSDF();
    generator.DetectAllPlateBoundaries();
    generator.GenerateContinentalLandmassSeeds();
    generator.GenerateContinentalHeightmapData();

    Image img = generator.GeneratePlateSDFMapImage();
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

Texture2D LandmassTesting::ReloadHeightmapCallback() {
    Image img = generator.GenerateCoastlineImage();
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

void LandmassTesting::TriggerViewportReloads() {
    ReloadTextureViewport(coastlineViewportId);
    ReloadTextureViewport(heightmapViewportId);
}

void LandmassTesting::Init() {
    generator.RunPlateAssignmentFloodFill();
    generator.CalculateContinentalPlateSDF();
    generator.DetectAllPlateBoundaries();
    generator.GenerateContinentalLandmassSeeds();
    generator.GenerateContinentalHeightmapData();

    Image sdfImg = generator.GeneratePlateSDFMapImage();
    coastlineTexture = LoadTextureFromImage(sdfImg);
    UnloadImage(sdfImg);

    Image coastImg = generator.GenerateCoastlineImage();
    heightmapTexture = LoadTextureFromImage(coastImg);
    UnloadImage(coastImg);

    coastlineViewportId = AddTextureViewport(coastlineTexture, "Continental SDF & Landmass Seeds (2D)", [this]() {
        return ReloadCoastlineCallback();
    }, true);

    heightmapViewportId = AddTextureViewport(heightmapTexture, "Generated Continental Coastline (2D)", [this]() {
        return ReloadHeightmapCallback();
    }, true);

    ConsoleLog::Get().AddLog(LogLevel::Info, "Continental Landmass Engine initialized.");
    ConsoleLog::Get().AddLog(LogLevel::Info, "Registered Continental SDF Seeds and Coastline Raylib Texture2D viewports.");
}

void LandmassTesting::Update(float deltaTime) {
    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if (IsKeyPressed(KEY_LEFT)) {
            if (generator.config.seed > 1) {
                generator.config.seed--;
                TriggerViewportReloads();
                ConsoleLog::Get().AddLog(LogLevel::Info, "Seed decremented to %d via [Left Arrow] key.", generator.config.seed);
            }
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            generator.config.seed++;
            TriggerViewportReloads();
            ConsoleLog::Get().AddLog(LogLevel::Info, "Seed incremented to %d via [Right Arrow] key.", generator.config.seed);
        }
    }
}

void LandmassTesting::DrawUI() {
    ImGui::TextDisabled("Continental Landmass Generator Controls");
    ImGui::Separator();

    bool changed = false;
    auto& config = generator.config;

    // 1. Grid Resolution & Random Seed
    if (ImGui::CollapsingHeader("Grid Resolution & Seed", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderInt("Grid Seed", &config.seed, 1, 9999)) changed = true;

        int currentRes = config.mapWidth;
        const char* resItems[] = { "32x32", "64x64", "128x128", "256x256", "512x512", "1024x1024" };
        int selectedRes = 3;
        if (currentRes == 32) selectedRes = 0;
        else if (currentRes == 64) selectedRes = 1;
        else if (currentRes == 128) selectedRes = 2;
        else if (currentRes == 256) selectedRes = 3;
        else if (currentRes == 512) selectedRes = 4;
        else if (currentRes == 1024) selectedRes = 5;

        if (ImGui::Combo("Grid Resolution", &selectedRes, resItems, IM_ARRAYSIZE(resItems))) {
            int newRes = 256;
            if (selectedRes == 0) newRes = 32;
            else if (selectedRes == 1) newRes = 64;
            else if (selectedRes == 2) newRes = 128;
            else if (selectedRes == 3) newRes = 256;
            else if (selectedRes == 4) newRes = 512;
            else if (selectedRes == 5) newRes = 1024;

            if (newRes != config.mapWidth) {
                config.mapWidth = newRes;
                config.mapHeight = newRes;
                changed = true;
            }
        }

        ImGui::Text("Total Cells: %d x %d = %d", config.mapWidth, config.mapHeight, config.mapWidth * config.mapHeight);
    }

    // 2. Tectonic Plate Assignment & Types
    if (ImGui::CollapsingHeader("Tectonic Growth & Plate Types", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderInt("Plate Seed Count", &config.numPlates, 2, 30)) changed = true;
        if (ImGui::SliderFloat("Land Plate Ratio", &config.landRatio, 0.10f, 0.90f, "%.2f")) changed = true;
        if (ImGui::SliderFloat("Plate Size Diversity", &config.plateSizeVariance, 0.0f, 1.0f, "%.2f")) changed = true;
    }

    // 3. Adaptive Continental Landmass Seed Spreading
    if (ImGui::CollapsingHeader("Adaptive Landmass Seed Spreading", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("Min Interior Seed SDF", &config.minInteriorSDF, 1.0f, 20.0f, "%.1f cells")) changed = true;
        if (ImGui::SliderFloat("Landmass Seed Spacing", &config.seedSpacing, 5.0f, 50.0f, "%.1f cells")) changed = true;
        if (ImGui::SliderFloat("Adaptive Radius Scale", &config.radiusScale, 0.20f, 2.00f, "%.2f")) changed = true;
        if (ImGui::SliderFloat("Max SDF Shading Depth", &config.maxSDFDepth, 5.0f, 100.0f, "%.1f cells")) changed = true;
    }

    // 4. Landmass Form & Multi-Seed Falloff Mask Controls
    if (ImGui::CollapsingHeader("Landmass Form & Multi-Seed Spacing", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* modeItems[] = { "Single Center (Classic)", "Multi-Seed Nearest (Voronoi)", "Multi-Seed Metaball (Organic)" };
        int currentMode = (int)config.falloffMode;
        if (ImGui::Combo("Falloff Mode", &currentMode, modeItems, IM_ARRAYSIZE(modeItems))) {
            config.falloffMode = (FalloffMode)currentMode;
            changed = true;
        }

        const char* falloffItems[] = {
            "Radial (Euclidean)",
            "Diamond (Superellipse)",
            "Starfish (Angular Wave)",
            "Random per Seed Center"
        };
        int currentFalloff = (int)config.falloffType;
        if (ImGui::Combo("Falloff Shape", &currentFalloff, falloffItems, IM_ARRAYSIZE(falloffItems))) {
            config.falloffType = (FalloffType)currentFalloff;
            changed = true;
        }

        if (ImGui::SliderFloat("Falloff Curve Exponent", &config.falloffPower, 0.5f, 4.0f, "%.2f")) changed = true;
    }

    // 5. FastNoise Lite Generator Controls
    if (ImGui::CollapsingHeader("FastNoise Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("Frequency (Grid-Invariant)", &config.frequency, 0.5f, 15.0f, "%.2f")) changed = true;
        if (ImGui::SliderFloat("Details", &config.details, 0.1f, 15.0f, "%.2f")) changed = true;
    }

    // 6. Coastline & Water Level
    if (ImGui::CollapsingHeader("Coastline & Water Level", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("Water Level", &config.waterLevel, 0.05f, 0.90f, "%.3f")) changed = true;
        if (ImGui::SliderFloat("Coastline Contour Width", &config.coastLineWidth, 0.002f, 0.04f, "%.3f")) changed = true;
    }

    // 7. Visualizer Layer Checkboxes
    if (ImGui::CollapsingHeader("Visualizer Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Checkbox("Render SDF Green/Blue Gradient", &config.drawSDFColors)) changed = true;
        if (ImGui::Checkbox("Draw Inter-Plate Boundaries (Orange)", &config.drawPlateBoundaries)) changed = true;
        if (ImGui::Checkbox("Draw Coastline Boundaries (Yellow)", &config.drawBoundaries)) changed = true;
        if (ImGui::Checkbox("Draw Tectonic Seeds (White)", &config.drawSeedPoints)) changed = true;
        if (ImGui::Checkbox("Draw Landmass Seeds (Magenta)", &config.drawLandmassSeeds)) changed = true;
    }

    ImGui::Spacing();
    ImGui::Checkbox("Auto-Reload Callbacks on Edit", &autoReloadOnSliderChange);

    ImGui::Spacing();
    if (ImGui::Button("Regenerate Continental Landmasses", ImVec2(-1, 34)) || (changed && autoReloadOnSliderChange)) {
        TriggerViewportReloads();
    }
}

void LandmassTesting::Shutdown() {
    ConsoleLog::Get().AddLog(LogLevel::Info, "LandmassTesting application shut down.");
}
