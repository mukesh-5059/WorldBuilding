#include "raylib/raylib.h"
#include "imgui/imgui.h"
#include "rlimgui/rlImGui.h"

int main() {
    // Initialize window
    InitWindow(800, 450, "WorldBuilder");
    SetTargetFPS(60);

    // Setup rlImGui
    rlImGuiSetup(true);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(DARKGRAY);

        // Render ImGui content
        rlImGuiBegin();
        ImGui::Begin("WorldBuilder Control Panel");
        ImGui::Text("Hello! Your CMake build and libraries are working!");
        ImGui::End();
        rlImGuiEnd();

        EndDrawing();
    }

    // Cleanup
    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
