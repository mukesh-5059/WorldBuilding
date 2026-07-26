#include "includes/utils.hpp"
#include "raylib/raymath.h"
#include <vector>
#include <cmath>

struct Triangle { int v1, v2, v3; };

static int GetMidpoint(int p1, int p2, std::vector<Vector3>& vertices) {
    Vector3 v1 = vertices[p1];
    Vector3 v2 = vertices[p2];
    Vector3 middle = { (v1.x + v2.x) * 0.5f, (v1.y + v2.y) * 0.5f, (v1.z + v2.z) * 0.5f };
    middle = Vector3Normalize(middle);

    vertices.push_back(middle);
    return (int)vertices.size() - 1;
}

Mesh GenerateIcosphereMesh(int subdivisions, float radius) {
    const float t = (1.0f + sqrtf(5.0f)) * 0.5f;

    int finalVertexCount = 20 * std::pow(4, subdivisions) - 8;
    std::vector<Vector3> vertices;
    vertices.reserve(finalVertexCount);

    vertices.push_back(Vector3Normalize({-1.0f,  t,  0.0f}));
    vertices.push_back(Vector3Normalize({ 1.0f,  t,  0.0f}));
    vertices.push_back(Vector3Normalize({-1.0f, -t,  0.0f}));
    vertices.push_back(Vector3Normalize({ 1.0f, -t,  0.0f}));
    vertices.push_back(Vector3Normalize({ 0.0f, -1.0f,  t}));
    vertices.push_back(Vector3Normalize({ 0.0f,  1.0f,  t}));
    vertices.push_back(Vector3Normalize({ 0.0f, -1.0f, -t}));
    vertices.push_back(Vector3Normalize({ 0.0f,  1.0f, -t}));
    vertices.push_back(Vector3Normalize({  t,  0.0f, -1.0f}));
    vertices.push_back(Vector3Normalize({  t,  0.0f,  1.0f}));
    vertices.push_back(Vector3Normalize({ -t,  0.0f, -1.0f}));
    vertices.push_back(Vector3Normalize({ -t,  0.0f,  1.0f}));

    std::vector<Triangle> faces = {
        { 0, 11,  5 }, { 0,  5,  1 }, { 0,  1,  7 }, { 0,  7, 10 }, { 0, 10, 11 },
        { 1,  5,  9 }, { 5, 11,  4 }, { 11, 10,  2 }, { 10,  7,  6 }, { 7,  1,  8 },
        { 3,  9,  4 }, { 3,  4,  2 }, { 3,  2,  6 }, { 3,  6,  8 }, { 3,  8,  9 },
        { 4,  9,  5 }, { 2,  4, 11 }, { 6,  2, 10 }, { 8,  6,  7 }, { 9,  8,  1 }
    };

    for (int i = 0; i < subdivisions; ++i) {
        std::vector<Triangle> newFaces;
        newFaces.reserve(faces.size() * 4);
        for (const auto& tri : faces) {
            int a = GetMidpoint(tri.v1, tri.v2, vertices);
            int b = GetMidpoint(tri.v2, tri.v3, vertices);
            int c = GetMidpoint(tri.v3, tri.v1, vertices);
            newFaces.push_back({ tri.v1, a, c });
            newFaces.push_back({ tri.v2, b, a });
            newFaces.push_back({ tri.v3, c, b });
            newFaces.push_back({ a, b, c });
        }
        faces = std::move(newFaces);
    }

    Mesh mesh = { 0 };
    mesh.vertexCount = (int)faces.size() * 3;
    mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    mesh.normals = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount * 2 * sizeof(float));
    mesh.colors = (unsigned char *)RL_MALLOC(mesh.vertexCount * 4 * sizeof(unsigned char));

    for (size_t tIndex = 0; tIndex < faces.size(); ++tIndex) {
        Triangle tri = faces[tIndex];
        Vector3 verts[3] = { vertices[tri.v1], vertices[tri.v2], vertices[tri.v3] };

        float u[3], v[3];

        for (int j = 0; j < 3; ++j) {
            mesh.vertices[(tIndex * 3 + j) * 3 + 0] = verts[j].x * radius;
            mesh.vertices[(tIndex * 3 + j) * 3 + 1] = verts[j].y * radius;
            mesh.vertices[(tIndex * 3 + j) * 3 + 2] = verts[j].z * radius;

            Vector3 norm = Vector3Normalize(verts[j]);
            mesh.normals[(tIndex * 3 + j) * 3 + 0] = norm.x;
            mesh.normals[(tIndex * 3 + j) * 3 + 1] = norm.y;
            mesh.normals[(tIndex * 3 + j) * 3 + 2] = norm.z;

            mesh.colors[(tIndex * 3 + j) * 4 + 0] = 255;
            mesh.colors[(tIndex * 3 + j) * 4 + 1] = 255;
            mesh.colors[(tIndex * 3 + j) * 4 + 2] = 255;
            mesh.colors[(tIndex * 3 + j) * 4 + 3] = 255;

            u[j] = (atan2f(norm.z, norm.x) / (2.0f * PI)) + 0.5f;
            v[j] = 0.5f - (asinf(norm.y) / PI);
        }

        // Fix UV wrap seam across longitude 180° boundary
        if (fabsf(u[0] - u[1]) > 0.5f || fabsf(u[1] - u[2]) > 0.5f || fabsf(u[0] - u[2]) > 0.5f) {
            for (int j = 0; j < 3; ++j) {
                if (u[j] < 0.5f) u[j] += 1.0f;
            }
        }

        for (int j = 0; j < 3; ++j) {
            mesh.texcoords[(tIndex * 3 + j) * 2 + 0] = u[j];
            mesh.texcoords[(tIndex * 3 + j) * 2 + 1] = v[j];
        }
    }

    UploadMesh(&mesh, false);
    return mesh;
}

Color GenerateRandomPlateColor(PlateType type, int seed) {
    unsigned int h = (unsigned int)10 * 1664525u + 1013904223u;
    float hue = 0.0f;
    float sat = 0.70f + ((float)(h & 0x3F) / 63.0f) * 0.25f;
    float val = 0.60f + ((float)((h >> 8) & 0x3F) / 63.0f) * 0.30f;

    if (type == PlateType::OCEANIC) {
        hue = 195.0f + ((float)((h >> 16) & 0x3F) / 63.0f) * 45.0f; // Blue/Cyan (195° to 240°)
    } else {
        hue = 75.0f + ((float)((h >> 16) & 0x3F) / 63.0f) * 60.0f;  // Green/Land (75° to 135°)
    }

    return ColorFromHSV(hue, sat, val);
}