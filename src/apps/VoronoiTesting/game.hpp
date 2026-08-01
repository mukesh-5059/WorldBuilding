#pragma once
#include "Application.hpp"
#include "CustomCamera.hpp"
#include "builder.hpp"
#include "raylib/raylib.h"

class VoronoiTesting : public Application {
private:
    CustomCamera* customCamera;
    Shader planetShader;

    Mesh sphereMesh;
    Model sphereModel;
    bool modelGenerated;

    int subdivisions;
    int lastSubdivisions;
    float radius;
    float lastRadius;

    Builder builder;

public:
    VoronoiTesting();
    ~VoronoiTesting() override;

    void Init() override;
    void Update(float deltaTime) override;
    void SceneDraw() override;
    void DrawUI() override;
    void Shutdown() override;

private:
    void RebuildMesh();
};
