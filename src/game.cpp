#include "includes/WorldBuilder.hpp"
#include "includes/TestShape.hpp"
#include "imgui/imgui.h"
#include "raylib/raylib.h"

WorldBuilder::WorldBuilder() : Application(1280, 720, "WorldBuilder") {}


void WorldBuilder::Init() {
    customCamera = new CustomCamera(Vector3{ 0.0f, 2.0f, 5.0f });

    wireframeShader = LoadShader("res/shaders/wireframe.vert", "res/shaders/wireframe.frag");
    lineColorLoc = GetShaderLocation(wireframeShader, "lineColor");
    fillColorLoc = GetShaderLocation(wireframeShader, "fillColor");
    thicknessLoc = GetShaderLocation(wireframeShader, "thickness");

    lineColArr[0] = 0.0f; lineColArr[1] = 1.0f; lineColArr[2] = 0.8f; lineColArr[3] = 1.0f;
    fillColArr[0] = 0.1f; fillColArr[1] = 0.15f; fillColArr[2] = 0.25f; fillColArr[3] = 0.6f;
    thickness = 1.5f;

    testModel = LoadModelFromMesh(CreateTestPyramid());
    testModel.materials[0].shader = wireframeShader;
}




void WorldBuilder::Update(float deltaTime) {
    customCamera->Update(deltaTime);

    SetShaderValue(wireframeShader, lineColorLoc, lineColArr, SHADER_UNIFORM_VEC4);
    SetShaderValue(wireframeShader, fillColorLoc, fillColArr, SHADER_UNIFORM_VEC4);
    SetShaderValue(wireframeShader, thicknessLoc, &thickness, SHADER_UNIFORM_FLOAT);
}




void WorldBuilder::Draw() {
    BeginMode3D(customCamera->camera);
        DrawGrid(10, 1.0f);
        DrawLine3D(Vector3{-5, 0, 0}, Vector3{5, 0, 0}, RED);
        DrawLine3D(Vector3{0, -5, 0}, Vector3{0, 5, 0}, GREEN);
        DrawLine3D(Vector3{0, 0, -5}, Vector3{0, 0, 5}, BLUE);
        DrawModel(testModel, Vector3{ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
    EndMode3D();
}




void WorldBuilder::DrawUI() {
    float screenWidth = (float)GetScreenWidth();
    float screenHeight = (float)GetScreenHeight();
    ImGui::SetNextWindowPos(ImVec2(screenWidth - 20, 20.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(150.0f, screenHeight - 30), ImVec2(screenWidth * 0.8f, screenHeight - 30));
    ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    if (ImGui::CollapsingHeader("Wireframe Settings")) {
        ImGui::SliderFloat("Line Thickness", &thickness, 0.5f, 10.0f, "%.1f px");
        ImGui::ColorEdit4("Line Color", lineColArr);
        ImGui::ColorEdit4("Fill Color", fillColArr);
    }
    if (ImGui::CollapsingHeader("Camera Settings")) customCamera->Gui();
    ImGui::End();
}




void WorldBuilder::Shutdown() {
    UnloadModel(testModel);
    UnloadShader(wireframeShader);
    delete customCamera;
}