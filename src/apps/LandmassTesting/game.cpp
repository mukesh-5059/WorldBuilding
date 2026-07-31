#include "LandmassTesting.hpp"
#include "ConsoleLog.hpp"
#include "imgui/imgui.h"

LandmassTesting::LandmassTesting()
    : Application(1280, 720, "2D Landmass Coastline Testing Engine") {
    show3DViewportTab = false; // Pure 2D texture viewport mode
}

LandmassTesting::~LandmassTesting() {
}

Texture2D LandmassTesting::ReloadCoastlineCallback() {
    generator.GenerateLandmassData();
    Image img = generator.GenerateCoastlineImage();
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

Texture2D LandmassTesting::ReloadHeightmapCallback() {
    Image img = generator.GenerateHeightmapImage();
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

void LandmassTesting::TriggerViewportReloads() {
    ReloadTextureViewport(coastlineViewportId);
    ReloadTextureViewport(heightmapViewportId);
}

void LandmassTesting::Init() {
    generator.GenerateLandmassData();

    Image coastImg = generator.GenerateCoastlineImage();
    coastlineTexture = LoadTextureFromImage(coastImg);
    UnloadImage(coastImg);

    Image hmImg = generator.GenerateHeightmapImage();
    heightmapTexture = LoadTextureFromImage(hmImg);
    UnloadImage(hmImg);

    coastlineViewportId = AddTextureViewport(coastlineTexture, "Coastline Map (2D)", [this]() {
        return ReloadCoastlineCallback();
    }, true);

    heightmapViewportId = AddTextureViewport(heightmapTexture, "Heightmap & Contour (2D)", [this]() {
        return ReloadHeightmapCallback();
    }, true);

    ConsoleLog::Get().AddLog(LogLevel::Info, "LandmassTesting application initialized.");
    ConsoleLog::Get().AddLog(LogLevel::Info, "Registered Coastline and Heightmap Raylib Texture2D viewports with callbacks.");
}

void LandmassTesting::Update(float deltaTime) {
    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if (IsKeyPressed(KEY_LEFT)) {
            if (generator.config.seed > 1) {
                generator.config.seed--;
                TriggerViewportReloads();
                ConsoleLog::Get().AddLog(LogLevel::Info, "Map Seed decremented to %d via [Left Arrow] key.", generator.config.seed);
            }
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            generator.config.seed++;
            TriggerViewportReloads();
            ConsoleLog::Get().AddLog(LogLevel::Info, "Map Seed incremented to %d via [Right Arrow] key.", generator.config.seed);
        }
    }
}

void LandmassTesting::DrawUI() {
    ImGui::TextDisabled("Coastline Generator Controls");
    ImGui::Separator();

    bool changed = false;
    auto& config = generator.config;

    // 1. Grid Resolution & Map Seed
    if (ImGui::CollapsingHeader("Grid & Map Seed", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderInt("Map Seed", &config.seed, 1, 9999)) changed = true;

        int currentRes = config.mapWidth;
        const char* resItems[] = { "128x128", "256x256", "512x512", "1024x1024" };
        int selectedRes = 1;
        if (currentRes == 128) selectedRes = 0;
        else if (currentRes == 256) selectedRes = 1;
        else if (currentRes == 512) selectedRes = 2;
        else if (currentRes == 1024) selectedRes = 3;

        if (ImGui::Combo("Grid Resolution", &selectedRes, resItems, IM_ARRAYSIZE(resItems))) {
            int newRes = 256;
            if (selectedRes == 0) newRes = 128;
            else if (selectedRes == 1) newRes = 256;
            else if (selectedRes == 2) newRes = 512;
            else if (selectedRes == 3) newRes = 1024;

            if (newRes != config.mapWidth) {
                config.mapWidth = newRes;
                config.mapHeight = newRes;
                changed = true;
            }
        }
    }

    // 2. Coastline & Water Level
    if (ImGui::CollapsingHeader("Coastline & Water Level", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("Water Level", &config.waterLevel, 0.05f, 0.90f, "%.3f")) changed = true;
        if (ImGui::SliderFloat("Coastline Contour Width", &config.coastLineWidth, 0.002f, 0.04f, "%.3f")) changed = true;
    }

    // 3. Multi-Seed Spacing & Falloff Mask Controls
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

        if (config.falloffMode != FalloffMode::SingleCenter) {
            if (ImGui::SliderInt("Seed Point Count", &config.seedCount, 2, 15)) changed = true;
            if (ImGui::SliderFloat("Seed Spread Radius", &config.seedSpread, 0.10f, 0.95f, "%.2f")) changed = true;
            if (ImGui::SliderFloat("Min Seed Radius", &config.seedMinRadius, 0.10f, 0.80f, "%.2f")) changed = true;
            if (ImGui::SliderFloat("Max Seed Radius", &config.seedMaxRadius, 0.20f, 1.20f, "%.2f")) changed = true;
        }

        if (ImGui::SliderFloat("Falloff Curve Exponent", &config.falloffPower, 0.5f, 4.0f, "%.2f")) changed = true;
    }

    // 4. FastNoiseLite Generator Controls (Grid-Invariant)
    if (ImGui::CollapsingHeader("FastNoise Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("Frequency (Grid-Invariant)", &config.frequency, 0.5f, 15.0f, "%.2f")) changed = true;
        if (ImGui::SliderFloat("Details", &config.details, 0.1f, 15.0f, "%.2f")) changed = true;
    }

    ImGui::Spacing();
    ImGui::Checkbox("Auto-Reload Callbacks on Edit", &autoReloadOnSliderChange);

    ImGui::Spacing();
    if (ImGui::Button("Regenerate & Invoke Callbacks", ImVec2(-1, 34)) || (changed && autoReloadOnSliderChange)) {
        TriggerViewportReloads();
    }
}

void LandmassTesting::Shutdown() {
    ConsoleLog::Get().AddLog(LogLevel::Info, "LandmassTesting application shut down.");
}
