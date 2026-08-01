#include "game.hpp"
#include "utils.hpp"
#include "imgui/imgui.h"
#include "ConsoleLog.hpp"

VoronoiTesting::VoronoiTesting()
    : Application(1280, 720, "Voronoi Testing"),
      customCamera(nullptr),
      modelGenerated(false),
      subdivisions(3),
      lastSubdivisions(3),
      radius(2.0f),
      lastRadius(2.0f) {
}

VoronoiTesting::~VoronoiTesting() {
}

void VoronoiTesting::Init() {
    customCamera = new CustomCamera(Vector3{ 0.0f, 2.0f, 5.0f });

    // Load shaders
    planetShader = LoadShader("res/shaders/planet.vert", "res/shaders/planet.frag");

    RebuildMesh();

    ConsoleLog::Get().AddLog(LogLevel::Info, "VoronoiTesting application initialized.");
}

void VoronoiTesting::RebuildMesh() {
    if (modelGenerated) {
        // Unload texture
        UnloadTexture(sphereModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
        UnloadModel(sphereModel);
        modelGenerated = false;
    }

    sphereMesh = GenerateSubdividedIcosahedron(subdivisions, radius);
    sphereModel = LoadModelFromMesh(sphereMesh);
    sphereModel.materials[0].shader = planetShader;
    
    // Set a default checkered texture
    Image image = GenImageChecked(1024, 512, 32, 32, RED, BLUE);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    
    sphereModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    modelGenerated = true;

    ConsoleLog::Get().AddLog(LogLevel::Info, "Rebuilt subdivided icosahedron: subdivisions=%d, radius=%.2f", subdivisions, radius);
}

void VoronoiTesting::Update(float deltaTime) {
    customCamera->Update(deltaTime, is3DViewportHovered);

    if (subdivisions != lastSubdivisions || radius != lastRadius) {
        RebuildMesh();
        lastSubdivisions = subdivisions;
        lastRadius = radius;
    }
}

void VoronoiTesting::SceneDraw() {
    BeginMode3D(customCamera->camera);
        DrawGrid(10, 1.0f);
        DrawLine3D(Vector3{-5, 0, 0}, Vector3{5, 0, 0}, RED);
        DrawLine3D(Vector3{0, -5, 0}, Vector3{0, 5, 0}, GREEN);
        DrawLine3D(Vector3{0, 0, -5}, Vector3{0, 0, 5}, BLUE);

        if (modelGenerated) {
            DrawModel(sphereModel, Vector3{ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
        }
    EndMode3D();
}

void VoronoiTesting::DrawUI() {
    if (ImGui::CollapsingHeader("Sphere Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderInt("Subdivisions", &subdivisions, 0, 6);
        ImGui::SliderFloat("Radius", &radius, 0.5f, 5.0f, "%.2f");
    }

    if (ImGui::CollapsingHeader("Builder Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputInt("Builder Seed", &builder.seed);
        if (ImGui::Button("Rebuild World", ImVec2(-1, 0))) {
            builder.Rebuild();
        }
    }
}

void VoronoiTesting::Shutdown() {
    if (modelGenerated) {
        UnloadTexture(sphereModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
        UnloadModel(sphereModel);
        modelGenerated = false;
    }
    delete customCamera;
    ConsoleLog::Get().AddLog(LogLevel::Info, "VoronoiTesting application shutdown.");
}
