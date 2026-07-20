#include "includes/WorldGenerator.hpp"
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

    for (size_t t = 0; t < faces.size(); ++t) {
        Triangle tri = faces[t];
        Vector3 verts[3] = { vertices[tri.v1], vertices[tri.v2], vertices[tri.v3] };

        for (int j = 0; j < 3; ++j) {
            mesh.vertices[(t * 3 + j) * 3 + 0] = verts[j].x * radius;
            mesh.vertices[(t * 3 + j) * 3 + 1] = verts[j].y * radius;
            mesh.vertices[(t * 3 + j) * 3 + 2] = verts[j].z * radius;

            Vector3 norm = Vector3Normalize(verts[j]);
            mesh.normals[(t * 3 + j) * 3 + 0] = norm.x;
            mesh.normals[(t * 3 + j) * 3 + 1] = norm.y;
            mesh.normals[(t * 3 + j) * 3 + 2] = norm.z;

            mesh.colors[(t * 3 + j) * 4 + 0] = 255;
            mesh.colors[(t * 3 + j) * 4 + 1] = 255;
            mesh.colors[(t * 3 + j) * 4 + 2] = 255;
            mesh.colors[(t * 3 + j) * 4 + 3] = 255;
        }

        mesh.texcoords[(t * 3 + 0) * 2 + 0] = 0.0f;
        mesh.texcoords[(t * 3 + 0) * 2 + 1] = 0.0f;
        mesh.texcoords[(t * 3 + 1) * 2 + 0] = 1.0f;
        mesh.texcoords[(t * 3 + 1) * 2 + 1] = 0.0f;
        mesh.texcoords[(t * 3 + 2) * 2 + 0] = 0.0f;
        mesh.texcoords[(t * 3 + 2) * 2 + 1] = 1.0f;
    }

    UploadMesh(&mesh, false);
    return mesh;
}
