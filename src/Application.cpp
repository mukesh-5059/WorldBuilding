#include "includes/Application.hpp"
#include "raylib/raylib.h"
#include "rlimgui/rlImGui.h"
#include "imgui/imgui.h"

Application::Application(int width, int height, const char* title)
    : width(width), height(height), title(title), running(true) {}

Application::~Application() {}

void Application::Run() {
    InitWindow(width, height, title);
    SetTargetFPS(60);
    rlImGuiSetup(true);

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

        BeginDrawing();
        ClearBackground(DARKGRAY);

        Draw();

        rlImGuiBegin();
        DrawUI();
        performanceGui();
        rlImGuiEnd();

        EndDrawing();
    }

    Shutdown();
    rlImGuiShutdown();
    CloseWindow();
}

void Application::performanceGui(){
    ImGui::SetNextWindowPos(ImVec2(980, 360), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 340), ImGuiCond_FirstUseEver);

    ImGui::Begin("Performance");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame Time: %.3f ms/frame ", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::SliderInt("Target FPS", &m_targetFps, 30, 300);
    ImGui::Separator();
    ImGui::PlotHistogram("Frame Times", m_frameTimeHistory, 100, m_frameTimeIndex, nullptr, 0.0f, 33.3f, ImVec2(0, 80));
    ImGui::End();
}