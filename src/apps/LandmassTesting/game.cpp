#include "LandmassTesting.hpp"
#include "helper.hpp"
#include "ConsoleLog.hpp"
#include "imgui/imgui.h"

LandmassTesting::LandmassTesting()
    : Application(1280, 720, "2D Tectonic Plate & Continental Landmass Engine") {
    show3DViewportTab = false; // Pure 2D texture viewport mode
}

LandmassTesting::~LandmassTesting() = default;

Texture2D LandmassTesting::ReloadCoastlineCallback() {
    generator.RunFullPipeline();
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

Texture2D LandmassTesting::ReloadRawNoiseCallback() {
    Image img = generator.GenerateRawNoiseImage();
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

Texture2D LandmassTesting::ReloadGradientCallback() {
    Image img = generator.GenerateGradientMapImage();
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

void LandmassTesting::TriggerViewportReloads() {
    ReloadTextureViewport(coastlineViewportId);
    ReloadTextureViewport(heightmapViewportId);
    ReloadTextureViewport(rawNoiseViewportId);
    ReloadTextureViewport(gradientViewportId);
}

void LandmassTesting::Init() {
    coastlineTexture = ReloadCoastlineCallback();
    heightmapTexture = ReloadHeightmapCallback();
    rawNoiseTexture = ReloadRawNoiseCallback();
    gradientTexture = ReloadGradientCallback();

    coastlineViewportId = AddTextureViewport(coastlineTexture, "Continental SDF & Landmass Seeds (2D)", [this]() {
        return ReloadCoastlineCallback();
    }, true);

    heightmapViewportId = AddTextureViewport(heightmapTexture, "Generated Continental Coastline (2D)", [this]() {
        return ReloadHeightmapCallback();
    }, true);

    rawNoiseViewportId = AddTextureViewport(rawNoiseTexture, "Generated FastNoise (2D)", [this]() {
        return ReloadRawNoiseCallback();
    }, true);

    gradientViewportId = AddTextureViewport(gradientTexture, "Map Tapering Gradient (2D)", [this]() {
        return ReloadGradientCallback();
    }, true);

    ConsoleLog::Get().AddLog(LogLevel::Info, "Continental Landmass Engine initialized.");
    ConsoleLog::Get().AddLog(LogLevel::Info, "Registered Continental SDF Seeds, Coastline, FastNoise, and Map Gradient Viewports.");
}

void LandmassTesting::Update(float deltaTime) {
    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if (IsKeyPressed(KEY_LEFT) && generator.config.seed > 1) {
            generator.config.seed--;
            TriggerViewportReloads();
            ConsoleLog::Get().AddLog(LogLevel::Info, "Seed decremented to %d via [Left Arrow] key.", generator.config.seed);
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

        const char* resItems[] = { "32x32", "64x64", "128x128", "256x256", "512x512", "1024x1024" };
        int selectedRes = GetResIndex(config.mapWidth);
        if (ImGui::Combo("Grid Resolution", &selectedRes, resItems, IM_ARRAYSIZE(resItems))) {
            int newRes = GetResFromIndex(selectedRes);
            if (newRes != config.mapWidth) {
                config.mapWidth = config.mapHeight = newRes;
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
        const char* biasModeItems[] = { "2D Map Gradient", "Low-Frequency Noise" };
        int currentBiasMode = (int)config.sdfBiasMode;
        if (ImGui::Combo("SDF Bias Mode", &currentBiasMode, biasModeItems, IM_ARRAYSIZE(biasModeItems))) {
            config.sdfBiasMode = (SDFBiasMode)currentBiasMode;
            changed = true;
        }

        if (config.sdfBiasMode == SDFBiasMode::MapGradient) {
            if (ImGui::SliderFloat("Vertical Tapering (North-to-South)", &config.plateTaperStrengthV, 0.0f, 1.0f, "%.2f")) changed = true;
            if (ImGui::SliderFloat("Horizontal Tapering (West-to-East)", &config.plateTaperStrengthH, 0.0f, 1.0f, "%.2f")) changed = true;
        } else if (config.sdfBiasMode == SDFBiasMode::LowFreqNoise) {
            if (ImGui::SliderFloat("Low-Freq Noise Strength", &config.lowFreqNoiseStrength, 0.0f, 1.5f, "%.2f")) changed = true;
            if (ImGui::SliderFloat("Low-Freq Noise Scale", &config.lowFreqNoiseFrequency, 0.1f, 5.0f, "%.2f")) changed = true;
        }

        if (ImGui::SliderFloat("Center Concentration (Power)", &config.concentrationPower, 0.1f, 5.0f, "%.2f")) changed = true;
        if (ImGui::SliderFloat("Min Interior Seed SDF", &config.minInteriorSDF, 1.0f, 20.0f, "%.1f cells")) changed = true;
        if (ImGui::SliderFloat("Landmass Seed Spacing", &config.seedSpacing, 5.0f, 50.0f, "%.1f cells")) changed = true;
        if (ImGui::SliderFloat("Adaptive Radius Scale", &config.radiusScale, 0.20f, 2.00f, "%.2f")) changed = true;
        if (ImGui::SliderFloat("Max SDF Shading Depth", &config.maxSDFDepth, 5.0f, 100.0f, "%.1f cells")) changed = true;
    }

    // 4. Landmass Form & Multi-Seed Falloff Mask Controls
    if (ImGui::CollapsingHeader("Landmass Form & Multi-Seed Spacing", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("Falloff Curve Exponent", &config.falloffPower, 0.5f, 4.0f, "%.2f")) changed = true;
    }



    // 5. FastNoise Lite Generator Controls
    if (ImGui::CollapsingHeader("FastNoise Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* fractalItems[] = { "None", "FBm", "Ridged", "PingPong" };
        int currentFractal = (int)config.fractalType;
        if (ImGui::Combo("Fractal Type", &currentFractal, fractalItems, IM_ARRAYSIZE(fractalItems))) {
            config.fractalType = (FastNoiseLite::FractalType)currentFractal;
            changed = true;
        }

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
