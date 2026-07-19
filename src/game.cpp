#include "includes/game.hpp"
#include "includes/WorldGenerator.hpp"
#include "imgui/imgui.h"
#include "raylib/raylib.h"

WorldBuilder::WorldBuilder() 
    : Application(1280, 720, "WorldBuilder"),
      modelGenerated(false) {}

void WorldBuilder::Init() {
    customCamera = new CustomCamera(Vector3{ 0.0f, 2.0f, 5.0f });

    wireframeShader = LoadShader("res/shaders/wireframe.vert", "res/shaders/wireframe.frag");
    lineColorLoc = GetShaderLocation(wireframeShader, "lineColor");
    fillColorLoc = GetShaderLocation(wireframeShader, "fillColor");
    thicknessLoc = GetShaderLocation(wireframeShader, "thickness");

    lineColArr[0] = 0.0f; lineColArr[1] = 0.0f; lineColArr[2] = 0.0f; lineColArr[3] = 1.0f;
    fillColArr[0] = 1.0f; fillColArr[1] = 1.0f; fillColArr[2] = 1.0f; fillColArr[3] = 0.6f;
    thickness = 0.5f;

    subdivisions = 3;
    lastSubdivisions = 3;
    radius = 1.5f;
    lastRadius = 1.5f;

    icosphereMesh = GenerateIcosphereMesh(subdivisions, radius);
    icosphereModel = LoadModelFromMesh(icosphereMesh);
    icosphereModel.materials[0].shader = wireframeShader;
    modelGenerated = true;
}

void WorldBuilder::Update(float deltaTime) {
    customCamera->Update(deltaTime);

    if (subdivisions != lastSubdivisions || radius != lastRadius) {
        if (modelGenerated) {
            UnloadModel(icosphereModel);
        }
        icosphereMesh = GenerateIcosphereMesh(subdivisions, radius);
        icosphereModel = LoadModelFromMesh(icosphereMesh);
        icosphereModel.materials[0].shader = wireframeShader;
        modelGenerated = true;

        lastSubdivisions = subdivisions;
        lastRadius = radius;
    }

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

        if (modelGenerated) {
            DrawModel(icosphereModel, Vector3{ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
        }
    EndMode3D();
}

void WorldBuilder::DrawUI() {
    float screenWidth = (float)GetScreenWidth();
    float screenHeight = (float)GetScreenHeight();
    ImGui::SetNextWindowPos(ImVec2(screenWidth - 20, 20.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(150.0f, screenHeight - 30), ImVec2(screenWidth * 0.8f, screenHeight - 30));
    ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    if (ImGui::CollapsingHeader("Icosphere Settings")) {
        ImGui::SliderInt("Subdivisions", &subdivisions, 0, 6);
        ImGui::SliderFloat("Radius", &radius, 0.5f, 5.0f, "%.2f");
    }
    if (ImGui::CollapsingHeader("Wireframe Settings")) {
        ImGui::SliderFloat("Line Thickness", &thickness, 0.5f, 10.0f, "%.1f px");
        ImGui::ColorEdit4("Line Color", lineColArr);
        ImGui::ColorEdit4("Fill Color", fillColArr);
    }
    if (ImGui::CollapsingHeader("Camera Settings")) customCamera->Gui();
    ImGui::End();
}

void WorldBuilder::Shutdown() {
    if (modelGenerated) {
        UnloadModel(icosphereModel);
    }
    UnloadShader(wireframeShader);
    delete customCamera;
}