#include "Application.hpp"
#include "ConsoleLog.hpp"
#include "raylib/raylib.h"
#include "rlimgui/rlImGui.h"


Application::Application(int width, int height, const char* title)
    : width(width), height(height), title(title), running(true),
      nextViewportId(1), is3DViewportHovered(false),
      inspectorWidth(340.0f), consoleHeight(200.0f) {
}

Application::~Application() {
    for (auto& vp : textureViewports) {
        if (vp.isLoaded && vp.ownsTexture && vp.texture.id > 0) {
            UnloadTexture(vp.texture);
            vp.isLoaded = false;
        }
    }
}

void Application::Run() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    int monitor = GetCurrentMonitor();
    int mWidth = GetMonitorWidth(monitor);
    int mHeight = GetMonitorHeight(monitor);
    if (mWidth > 0 && mHeight > 0) {
        width = mWidth;
        height = mHeight;
    }

    InitWindow(width, height, title);
    ToggleFullscreen();

    SetTargetFPS(60);
    rlImGuiSetup(true);

    SetTraceLogCallback(RaylibTraceLogCallback);
    ConsoleLog::Get().AddLog(LogLevel::Info, "WorldBuilder Editor Console initialized.");

    sceneRenderTexture = LoadRenderTexture(width, height);
    SetTextureFilter(sceneRenderTexture.texture, TEXTURE_FILTER_BILINEAR);

    Init();
    m_targetFps = 60;
    m_lastTargetFps = 60;
    for (int i = 0; i < 100; ++i) m_frameTimeHistory[i] = 0.0f;
    m_frameTimeIndex = 0;

    while (!WindowShouldClose() && running) {
        float deltaTime = GetFrameTime();

        if (m_targetFps != m_lastTargetFps) {
            SetTargetFPS(m_targetFps);
            m_lastTargetFps = m_targetFps;
        }

        m_frameTimeHistory[m_frameTimeIndex] = deltaTime * 1000.0f;
        m_frameTimeIndex = (m_frameTimeIndex + 1) % 100;
        
        Update(deltaTime);

        // 1. Offscreen 3D Scene Rendering to Texture
        BeginTextureMode(sceneRenderTexture);
            ClearBackground(DARKGRAY);
            SceneDraw();
        EndTextureMode();

        // 2. Pure ImGui-Driven Fullscreen Editor Layout
        BeginDrawing();
            ClearBackground(BLACK);

            rlImGuiBegin();

            // Render Full Editor GUI
            editorGui();

            // Floating Performance GUI Window
            performanceGui();

            rlImGuiEnd();

        EndDrawing();
    }

    UnloadRenderTexture(sceneRenderTexture);
    Shutdown();
    rlImGuiShutdown();
    CloseWindow();
}


void Application::SetRenderResolution(int newWidth, int newHeight) {
    if (newWidth < 256) newWidth = 256;
    if (newHeight < 144) newHeight = 144;

    if (sceneRenderTexture.texture.width != newWidth || sceneRenderTexture.texture.height != newHeight) {
        UnloadRenderTexture(sceneRenderTexture);
        sceneRenderTexture = LoadRenderTexture(newWidth, newHeight);
        SetTextureFilter(sceneRenderTexture.texture, TEXTURE_FILTER_BILINEAR);
        width = newWidth;
        height = newHeight;
    }
}