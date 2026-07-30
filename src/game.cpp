#include "includes/game.hpp"
#include "includes/WorldGenerator.hpp"
#include "imgui/imgui.h"
#include "raylib/raylib.h"
#include "raylib/raymath.h"

WorldBuilder::WorldBuilder() 
    : Application(1280, 720, "WorldBuilder"),
      modelGenerated(false),
      uiLonResIndex(0) {}

void WorldBuilder::RebuildPlotter() {
    plotter.Rebuild(plotter.cubemapFaceRes, radius, plotter.texWidth, plotter.texHeight);

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

void WorldBuilder::On3DViewportClicked(Vector2 mouseNormInViewport) {
    Vector2 windowPos = { mouseNormInViewport.x * (float)GetScreenWidth(), mouseNormInViewport.y * (float)GetScreenHeight() };
    Ray ray = GetMouseRay(windowPos, customCamera->camera);
    RayCollision collision = GetRayCollisionSphere(ray, Vector3{ 0.0f, 0.0f, 0.0f }, radius);

    if (collision.hit) {
        Vector3 hitDir = Vector3Normalize(collision.point);
        int cellId = GetCellIdFrom3DVector(hitDir, plotter.cubemapFaceRes);
        if (cellId >= 0 && cellId < (int)plotter.cellPlateOwner.size()) {
            plotter.selectedPlateId = plotter.cellPlateOwner[cellId];
        } else {
            plotter.selectedPlateId = -1;
        }
    } else {
        plotter.selectedPlateId = -1;
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

        // Draw 3D Overlays for Selected Plate (Euler Pole Axis & Velocity Vectors)
        if (plotter.selectedPlateId >= 0 && plotter.selectedPlateId < (int)plotter.plates.size()) {
            const auto& selPlate = plotter.plates[plotter.selectedPlateId];
            Vector3 polePos = Vector3Scale(selPlate.eulerPole, radius * 1.35f);
            Vector3 negPolePos = Vector3Scale(selPlate.eulerPole, -radius * 1.35f);

            // Draw Euler Rotation Axis line (Yellow) and Pole Markers
            DrawLine3D(negPolePos, polePos, YELLOW);
            DrawSphere(polePos, 0.08f, YELLOW);
            DrawSphere(negPolePos, 0.06f, DARKGRAY);

            // Draw surface velocity vectors for selected plate
            int N = plotter.cubemapFaceRes;
            int total = (int)plotter.cellPlateOwner.size();
            int step = (total > 50000) ? 12 : 4;
            for (int c = 0; c < total; c += step) {
                if (plotter.cellPlateOwner[c] == plotter.selectedPlateId) {
                    Vector3 cellDir = GetCell3DVector(c, N);
                    Vector3 cellPos = Vector3Scale(cellDir, radius * 1.01f);
                    Vector3 rotVel = Vector3CrossProduct(Vector3Scale(selPlate.eulerPole, selPlate.angularSpeed), cellDir);
                    DrawLine3D(cellPos, Vector3Add(cellPos, Vector3Scale(rotVel, 8.0f)), YELLOW);
                }
            }
        }
    EndMode3D();
}

void WorldBuilder::DrawUI() {
    if (ImGui::CollapsingHeader("Selected Plate Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (plotter.selectedPlateId >= 0 && plotter.selectedPlateId < (int)plotter.plates.size()) {
            const auto& p = plotter.plates[plotter.selectedPlateId];
            ImGui::Text("Plate ID: #%d", p.id);
            ImGui::Text("Type: %s", (p.type == PlateType::CONTINENTAL) ? "CONTINENTAL (Land)" : "OCEANIC (Water)");
            ImGui::Text("Growth Step Bias: %.2f", p.growthBias);
            ImGui::Separator();
            ImGui::Text("Euler Rotation Pole: (%.2f, %.2f, %.2f)", p.eulerPole.x, p.eulerPole.y, p.eulerPole.z);
            ImGui::Text("Angular Speed: %.4f rad/frame", p.angularSpeed);

            int cellCount = 0;
            for (int owner : plotter.cellPlateOwner) {
                if (owner == p.id) cellCount++;
            }
            float pct = (float)cellCount / (float)plotter.cellPlateOwner.size() * 100.0f;
            ImGui::Text("Coverage: %d cells (%.2f%% of planet)", cellCount, pct);

            ImGui::Spacing();
            if (ImGui::Button("Deselect Plate", ImVec2(-1, 0))) {
                plotter.selectedPlateId = -1;
            }
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click any plate on the 3D globe to inspect its Euler Pole, velocity vectors, and properties.");
        }
    }

    if (ImGui::CollapsingHeader("Cubemap Grid Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderInt("Cubemap Face Res", &plotter.cubemapFaceRes, 4, 256)) {
            RebuildPlotter();
        }
    }

    if (ImGui::CollapsingHeader("Tectonic Plate Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderInt("Export Width", &plotter.texWidth, 512, 4096, "%d px")) {
            if (plotter.texWidth % 2 != 0) plotter.texWidth++;
            plotter.texHeight = plotter.texWidth / 2;
            RebuildPlotter();
        }
        ImGui::TextDisabled("Export Res: %d x %d px (2:1 aspect ratio)", plotter.texWidth, plotter.texHeight);
        ImGui::Spacing();

        if (ImGui::SliderInt("Plate Count", &plotter.numPlates, 2, 140)) {
            RebuildPlotter();
        }

        if (ImGui::SliderInt("Landmass ratio", &plotter.landToWaterRatio, 0, 10)) {
            RebuildPlotter();
        }

        if (ImGui::SliderFloat("Land Threshold", &plotter.landThreshold, 0.0f, 1.0f, "%.2f")) {
            RebuildPlotter();
        }

        if (ImGui::SliderFloat("Plate Size variance", &plotter.plateSizeVariance, 0.0f, 1.0f, "%.2f")) {
            RebuildPlotter();
        }

        if (ImGui::SliderFloat("Plate border jitter", &plotter.borderJitterStrength, 0.0f, 2.0f, "%.2f")) {
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