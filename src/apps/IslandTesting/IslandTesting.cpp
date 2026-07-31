#include "IslandTesting.hpp"
#include "ConsoleLog.hpp"
#include "imgui/imgui.h"
#include "raylib/raymath.h"
#include "rlimgui/rlImGui.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

IslandTesting::IslandTesting()
    : Application(1280, 720, "2D Island Coastline Testing Engine") {
    show3DViewportTab = false; // Pure 2D texture viewport mode
}

IslandTesting::~IslandTesting() {
}

float IslandTesting::EvaluateSeedFalloff(float dx, float dy, const SeedPoint& sp) {
    float r = std::max(0.01f, sp.radius);
    FalloffType shape = sp.shape;

    if (shape == FalloffType::Radial) {
        float normD = sqrtf(dx * dx + dy * dy) / r;
        return powf(std::max(0.0f, 1.0f - normD), config.falloffPower);
    } 
    else if (shape == FalloffType::Diamond) {
        // 1. Rotation by per-seed angle theta
        float rad = sp.diamondAngle * (PI / 180.0f);
        float cosA = cosf(rad);
        float sinA = sinf(rad);
        float rx = dx * cosA - dy * sinA;
        float ry = dx * sinA + dy * cosA;

        // 2. Per-seed Aspect Ratio stretching (elongation / scale)
        float aspect = std::max(0.1f, sp.diamondAspect);
        float normX = fabsf(rx) / (r * aspect);
        float normY = fabsf(ry) / (r / aspect);

        // 3. Hardcoded Pinch L_q norm exponent q = 1.19
        float q = std::clamp(HARDCODED_DIAMOND_PINCH, 0.4f, 2.0f);
        float normD = powf(powf(normX, q) + powf(normY, q), 1.0f / q);

        return powf(std::max(0.0f, 1.0f - normD), config.falloffPower);
    } 
    else if (shape == FalloffType::Starfish) {
        float angle = atan2f(dy, dx);
        // Radius-dependent arm dampening (hardcoded dampening exponent = 2.24)
        float normRadiusRatio = std::clamp(r / std::max(0.01f, config.seedMaxRadius), 0.1f, 1.0f);
        float effectiveAmp = sp.starAmp * powf(normRadiusRatio, HARDCODED_STARFISH_DAMPENING);
        float modulation = 1.0f + effectiveAmp * sinf(sp.starArms * angle);
        float dist = sqrtf(dx * dx + dy * dy) * modulation;
        float normD = dist / r;
        return powf(std::max(0.0f, 1.0f - normD), config.falloffPower);
    }

    float normD = sqrtf(dx * dx + dy * dy) / r;
    return powf(std::max(0.0f, 1.0f - normD), config.falloffPower);
}

std::vector<SeedPoint> IslandTesting::GenerateSeedPoints() {
    std::vector<SeedPoint> points;

    auto HashRnd = [this](int id, int offset) {
        int n = id * 374761393 + config.seed * 668265263 + offset * 144662241;
        n = (n ^ (n >> 13)) * 1274126177;
        return (float)(n & 0x7fffffff) / (float)0x7fffffff;
    };

    FalloffType availableShapes[] = {
        FalloffType::Radial,
        FalloffType::Diamond,
        FalloffType::Starfish
    };
    int numAvailable = 3;

    int minArms = 3;
    int maxArms = std::max(3, (int)roundf(HARDCODED_STARFISH_ARMS));
    int armsRange = std::max(1, maxArms - minArms + 1);

    if (config.falloffMode == FalloffMode::SingleCenter || config.seedCount <= 1) {
        FalloffType shape = config.falloffType;
        if (shape == FalloffType::RandomPerSeed) {
            shape = availableShapes[(int)(HashRnd(0, 4) * numAvailable) % numAvailable];
        }
        float starArms = (float)(minArms + (int)(HashRnd(0, 5) * armsRange) % armsRange);
        float diamondAngle = HashRnd(0, 7) * 90.0f;
        float diamondAspect = 0.6f + HashRnd(0, 8) * 1.0f; // 0.6 to 1.6 aspect ratio
        points.push_back(SeedPoint{ 0.0f, 0.0f, 1.0f, shape, starArms, HARDCODED_STARFISH_AMP, diamondAngle, diamondAspect });
        return points;
    }

    for (int i = 0; i < config.seedCount; ++i) {
        float angle = HashRnd(i, 1) * 2.0f * PI;
        float dist = sqrtf(HashRnd(i, 2)) * config.seedSpread;
        float sx = cosf(angle) * dist;
        float sy = sinf(angle) * dist;
        float r = config.seedMinRadius + HashRnd(i, 3) * std::max(0.01f, config.seedMaxRadius - config.seedMinRadius);

        FalloffType shape = config.falloffType;
        if (shape == FalloffType::RandomPerSeed) {
            int shapeIdx = (int)(HashRnd(i, 4) * numAvailable) % numAvailable;
            shape = availableShapes[shapeIdx];
        }

        // Randomize number of arms per starfish seed in range [3, HARDCODED_STARFISH_ARMS]
        float starArms = (float)(minArms + (int)(HashRnd(i, 5) * armsRange) % armsRange);
        float starAmp = std::clamp(HARDCODED_STARFISH_AMP * (0.8f + HashRnd(i, 6) * 0.4f), 0.05f, 0.60f);

        // Randomize diamond rotation angle [0..90 deg] and aspect ratio [0.6..1.6] per seed center
        float diamondAngle = HashRnd(i, 7) * 90.0f;
        float diamondAspect = 0.6f + HashRnd(i, 8) * 1.0f;

        points.push_back(SeedPoint{ sx, sy, r, shape, starArms, starAmp, diamondAngle, diamondAspect });
    }
    return points;
}

float IslandTesting::CalculateMultiSeedMask(float nx, float ny, const std::vector<SeedPoint>& seeds) {
    if (seeds.empty()) return 1.0f;

    if (config.falloffMode == FalloffMode::SingleCenter) {
        return EvaluateSeedFalloff(nx - seeds[0].x, ny - seeds[0].y, seeds[0]);
    }

    if (config.falloffMode == FalloffMode::MultiSeedNearest) {
        float maxVal = 0.0f;
        for (const auto& sp : seeds) {
            float val = EvaluateSeedFalloff(nx - sp.x, ny - sp.y, sp);
            if (val > maxVal) {
                maxVal = val;
            }
        }
        return maxVal;
    }

    if (config.falloffMode == FalloffMode::MultiSeedMetaball) {
        float totalPotential = 0.0f;
        for (const auto& sp : seeds) {
            float val = EvaluateSeedFalloff(nx - sp.x, ny - sp.y, sp);
            totalPotential += val;
        }
        return std::clamp(totalPotential, 0.0f, 1.0f);
    }

    return 1.0f;
}

void IslandTesting::GenerateIslandData() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    heightmap.assign(w * h, 0.0f);

    std::vector<SeedPoint> seeds = GenerateSeedPoints();

    FastNoiseLite noise;
    noise.SetSeed(config.seed);
    noise.SetNoiseType(HARDCODED_NOISE_TYPE);     // Hardcoded OpenSimplex2
    noise.SetFractalType(HARDCODED_FRACTAL_TYPE); // Hardcoded Ridged
    noise.SetFractalOctaves(HARDCODED_OCTAVES);   // Hardcoded Octaves = 4
    noise.SetFractalGain(NOISE_GAIN);             // Hardcoded Gain = 0.4f
    noise.SetFractalLacunarity(NOISE_LACUNARITY); // Hardcoded Lacunarity = 2.1f

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            // Normalized coordinates [0..1] for grid-resolution invariance
            float u = (float)x / (float)w;
            float v = (float)y / (float)h;

            // Normalized centered coordinates [-1..1] for falloff mask
            float nx = u * 2.0f - 1.0f;
            float ny = v * 2.0f - 1.0f;

            float mask = CalculateMultiSeedMask(nx, ny, seeds);

            // Frequency & Details multiplier relative to normalized UV coordinates (resolution-independent!)
            float noiseScale = config.frequency * config.details * 10.0f;
            float rawNoise = noise.GetNoise(u * noiseScale, v * noiseScale);
            float normalizedNoise = (rawNoise + 1.0f) * 0.5f;

            heightmap[y * w + x] = std::clamp(normalizedNoise * mask, 0.0f, 1.0f);
        }
    }
}

Image IslandTesting::GenerateCoastlineImage() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    Image img = GenImageColor(w, h, BLACK);

    Color oceanColor     = Color{ 21, 101, 192, 255 };  // Deep Blue
    Color landColor      = Color{ 46, 125, 50, 255 };   // Emerald Green
    Color coastlineColor = Color{ 255, 215, 0, 255 };   // Bright Gold Outline

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float val = heightmap[y * w + x];
            Color c;

            if (fabsf(val - config.waterLevel) <= config.coastLineWidth) {
                c = coastlineColor; // Highlight coastline boundary edge
            } else if (val > config.waterLevel) {
                c = landColor;      // Landmass
            } else {
                c = oceanColor;     // Ocean
            }
            ImageDrawPixel(&img, x, y, c);
        }
    }
    return img;
}

Image IslandTesting::GenerateHeightmapImage() {
    int w = config.mapWidth;
    int h = config.mapHeight;
    Image img = GenImageColor(w, h, BLACK);

    Color contourColor = Color{ 0, 225, 255, 255 }; // Bright Cyan Coastline Overlay

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float val = heightmap[y * w + x];
            Color c;

            if (fabsf(val - config.waterLevel) <= config.coastLineWidth) {
                c = contourColor;
            } else {
                unsigned char byteVal = (unsigned char)(std::clamp(val, 0.0f, 1.0f) * 255.0f);
                c = Color{ byteVal, byteVal, byteVal, 255 };
            }
            ImageDrawPixel(&img, x, y, c);
        }
    }
    return img;
}

Texture2D IslandTesting::ReloadCoastlineCallback() {
    GenerateIslandData();
    Image img = GenerateCoastlineImage();
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

Texture2D IslandTesting::ReloadHeightmapCallback() {
    Image img = GenerateHeightmapImage();
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

void IslandTesting::TriggerViewportReloads() {
    ReloadTextureViewport(coastlineViewportId);
    ReloadTextureViewport(heightmapViewportId);
}

void IslandTesting::Init() {
    GenerateIslandData();

    Image coastImg = GenerateCoastlineImage();
    coastlineTexture = LoadTextureFromImage(coastImg);
    UnloadImage(coastImg);

    Image hmImg = GenerateHeightmapImage();
    heightmapTexture = LoadTextureFromImage(hmImg);
    UnloadImage(hmImg);

    coastlineViewportId = AddTextureViewport(coastlineTexture, "Coastline Map (2D)", [this]() {
        return ReloadCoastlineCallback();
    }, true);

    heightmapViewportId = AddTextureViewport(heightmapTexture, "Heightmap & Contour (2D)", [this]() {
        return ReloadHeightmapCallback();
    }, true);

    ConsoleLog::Get().AddLog(LogLevel::Info, "IslandTesting initialized with hardcoded shape refinement constants.");
    ConsoleLog::Get().AddLog(LogLevel::Info, "Registered Coastline and Heightmap Raylib Texture2D viewports with callbacks.");
}

void IslandTesting::Update(float deltaTime) {
    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if (IsKeyPressed(KEY_LEFT)) {
            if (config.seed > 1) {
                config.seed--;
                TriggerViewportReloads();
                ConsoleLog::Get().AddLog(LogLevel::Info, "Map Seed decremented to %d via [Left Arrow] key.", config.seed);
            }
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            config.seed++;
            TriggerViewportReloads();
            ConsoleLog::Get().AddLog(LogLevel::Info, "Map Seed incremented to %d via [Right Arrow] key.", config.seed);
        }
    }
}

void IslandTesting::DrawUI() {
    ImGui::TextDisabled("Coastline Generator Controls");
    ImGui::Separator();

    bool changed = false;

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
    if (ImGui::CollapsingHeader("Island Form & Multi-Seed Spacing", ImGuiTreeNodeFlags_DefaultOpen)) {
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

void IslandTesting::Shutdown() {
    ConsoleLog::Get().AddLog(LogLevel::Info, "IslandTesting application shut down.");
}
