#include "includes/game.hpp"
#include "includes/WorldGenerator.hpp"
#include "imgui/imgui.h"
#include "raylib/raylib.h"

WorldBuilder::WorldBuilder() 
    : Application(1280, 720, "WorldBuilder"),
      modelGenerated(false),
      uiLonResIndex(0) {}

void WorldBuilder::RebuildPlotter() {
    plotter.Rebuild(plotter.cubemapFaceRes, radius);

    if (modelGenerated && plotter.textureLoaded) {
        icosphereModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = plotter.texture;
    }
}

void WorldBuilder::Init() {
    customCamera = new CustomCamera(Vector3{ 0.0f, 2.0f, 5.0f });

    planetShader = LoadShader("res/shaders/planet.vert", "res/shaders/planet.frag");
    wireframeShader = LoadShader("res/shaders/wireframe.vert", "res/shaders/wireframe.frag");

    subdivisions = 4;
    lastSubdivisions = 4;
    radius = 2.0f;
    lastRadius = 2.0f;

    icosphereMesh = GenerateIcosphereMesh(subdivisions, radius);
    icosphereModel = LoadModelFromMesh(icosphereMesh);
    icosphereModel.materials[0].shader = planetShader;
    modelGenerated = true;

    RebuildPlotter();
}

void WorldBuilder::Update(float deltaTime) {
    customCamera->Update(deltaTime, is3DViewportHovered);

    if (subdivisions != lastSubdivisions || radius != lastRadius) {
        if (modelGenerated) {
            UnloadModel(icosphereModel);
        }
        icosphereMesh = GenerateIcosphereMesh(subdivisions, radius);
        icosphereModel = LoadModelFromMesh(icosphereMesh);
        icosphereModel.materials[0].shader = planetShader;
        modelGenerated = true;

        lastSubdivisions = subdivisions;
        lastRadius = radius;
        RebuildPlotter();
    }
}

void WorldBuilder::SceneDraw() {
    BeginMode3D(customCamera->camera);
        DrawGrid(10, 1.0f);
        DrawLine3D(Vector3{-5, 0, 0}, Vector3{5, 0, 0}, RED);
        DrawLine3D(Vector3{0, -5, 0}, Vector3{0, 5, 0}, GREEN);
        DrawLine3D(Vector3{0, 0, -5}, Vector3{0, 0, 5}, BLUE);

        if (plotter.showIcosphereBase && modelGenerated) {
            DrawModel(icosphereModel, Vector3{ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
        }
    EndMode3D();
}

void WorldBuilder::DrawUI() {
    if (ImGui::CollapsingHeader("Cubemap Grid Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderInt("Cubemap Face Res", &plotter.cubemapFaceRes, 4, 256)) {
            RebuildPlotter();
        }
    }

    if (ImGui::CollapsingHeader("Tectonic Plate Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderInt("Plate Count", &plotter.numPlates, 2, 40)) {
            RebuildPlotter();
        }

        if (ImGui::Checkbox("Draw Plate Boundaries", &plotter.drawBoundaries)) {
            plotter.RenderPointsToEquirectangularTexture();
        }

        if (ImGui::Button("Regenerate Tectonic Plates", ImVec2(-1, 0))) {
            RebuildPlotter();
        }

        if (ImGui::Button("Export Texture PNG", ImVec2(-1, 0))) {
            MakeDirectory("generated");
            if (plotter.ExportTextureImage("generated/polar_point_texture.png")) {
                TraceLog(LOG_INFO, "Successfully exported generated/polar_point_texture.png");
            }
        }

        if (ImGui::Button("Export & Open in New Tab", ImVec2(-1, 0))) {
            MakeDirectory("generated");
            if (plotter.ExportTextureImage("generated/polar_point_texture.png")) {
                AddTextureViewport("generated/polar_point_texture.png");
            }
        }
    }

    if (ImGui::CollapsingHeader("Icosphere Settings")) {
        ImGui::SliderInt("Subdivisions", &subdivisions, 0, 8);
        if (ImGui::SliderFloat("Radius", &radius, 0.5f, 15.0f, "%.2f")) {
            RebuildPlotter();
        }
    }
    if (ImGui::CollapsingHeader("Camera Settings")) customCamera->Gui();
}

void WorldBuilder::Shutdown() {
    plotter.Unload();
    if (modelGenerated) {
        UnloadModel(icosphereModel);
    }
    UnloadShader(planetShader);
    UnloadShader(wireframeShader);
    delete customCamera;
}