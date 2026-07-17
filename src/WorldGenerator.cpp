#include "includes/WorldGenerator.hpp"
#include "raylib/raymath.h"
#include <vector>
#include <map>
#include <cmath>

struct Triangle { int v1, v2, v3; };

static int GetMidpoint(int p1, int p2, std::vector<Vector3>& vertices, std::map<std::pair<int, int>, int>& cache) {
    auto key = std::make_pair(std::min(p1, p2), std::max(p1, p2));
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;

    Vector3 v1 = vertices[p1];
    Vector3 v2 = vertices[p2];
    Vector3 middle = { (v1.x + v2.x) * 0.5f, (v1.y + v2.y) * 0.5f, (v1.z + v2.z) * 0.5f };
    middle = Vector3Normalize(middle);

    vertices.push_back(middle);
    int newIndex = (int)vertices.size() - 1;
    cache[key] = newIndex;
    return newIndex;
}

Mesh CreateIcosphere(int subdivisions, float radius) {
    const float t = (1.0f + sqrtf(5.0f)) * 0.5f;

    std::vector<Vector3> vertices = {
        Vector3Normalize({-1.0f,  t,  0.0f}),
        Vector3Normalize({ 1.0f,  t,  0.0f}),
        Vector3Normalize({-1.0f, -t,  0.0f}),
        Vector3Normalize({ 1.0f, -t,  0.0f}),
        Vector3Normalize({ 0.0f, -1.0f,  t}),
        Vector3Normalize({ 0.0f,  1.0f,  t}),
        Vector3Normalize({ 0.0f, -1.0f, -t}),
        Vector3Normalize({ 0.0f,  1.0f, -t}),
        Vector3Normalize({  t,  0.0f, -1.0f}),
        Vector3Normalize({  t,  0.0f,  1.0f}),
        Vector3Normalize({ -t,  0.0f, -1.0f}),
        Vector3Normalize({ -t,  0.0f,  1.0f})
    };

    std::vector<Triangle> faces = {
        { 0, 11,  5 }, { 0,  5,  1 }, { 0,  1,  7 }, { 0,  7, 10 }, { 0, 10, 11 },
        { 1,  5,  9 }, { 5, 11,  4 }, { 11, 10,  2 }, { 10,  7,  6 }, { 7,  1,  8 },
        { 3,  9,  4 }, { 3,  4,  2 }, { 3,  2,  6 }, { 3,  6,  8 }, { 3,  8,  9 },
        { 4,  9,  5 }, { 2,  4, 11 }, { 6,  2, 10 }, { 8,  6,  7 }, { 9,  8,  1 }
    };

    std::map<std::pair<int, int>, int> midpointCache;

    for (int i = 0; i < subdivisions; ++i) {
        std::vector<Triangle> newFaces;
        for (const auto& tri : faces) {
            int a = GetMidpoint(tri.v1, tri.v2, vertices, midpointCache);
            int b = GetMidpoint(tri.v2, tri.v3, vertices, midpointCache);
            int c = GetMidpoint(tri.v3, tri.v1, vertices, midpointCache);

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
    mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount * 2 * sizeof(float));
    mesh.normals = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));

    for (size_t i = 0; i < faces.size(); ++i) {
        Vector3 v1 = vertices[faces[i].v1];
        Vector3 v2 = vertices[faces[i].v2];
        Vector3 v3 = vertices[faces[i].v3];

        Vector3 edge1 = { v2.x - v1.x, v2.y - v1.y, v2.z - v1.z };
        Vector3 edge2 = { v3.x - v1.x, v3.y - v1.y, v3.z - v1.z };
        Vector3 normal = {
            edge1.y * edge2.z - edge1.z * edge2.y,
            edge1.z * edge2.x - edge1.x * edge2.z,
            edge1.x * edge2.y - edge1.y * edge2.x
        };
        float len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (len > 0.0f) {
            normal.x /= len;
            normal.y /= len;
            normal.z /= len;
        }

        Vector3 triVerts[3] = { v1, v2, v3 };
        for (int j = 0; j < 3; ++j) {
            mesh.vertices[(i * 3 + j) * 3 + 0] = triVerts[j].x * radius;
            mesh.vertices[(i * 3 + j) * 3 + 1] = triVerts[j].y * radius;
            mesh.vertices[(i * 3 + j) * 3 + 2] = triVerts[j].z * radius;

            mesh.normals[(i * 3 + j) * 3 + 0] = normal.x;
            mesh.normals[(i * 3 + j) * 3 + 1] = normal.y;
            mesh.normals[(i * 3 + j) * 3 + 2] = normal.z;
        }

        mesh.texcoords[(i * 3 + 0) * 2 + 0] = 1.0f;
        mesh.texcoords[(i * 3 + 0) * 2 + 1] = 0.0f;

        mesh.texcoords[(i * 3 + 1) * 2 + 0] = 0.0f;
        mesh.texcoords[(i * 3 + 1) * 2 + 1] = 1.0f;

        mesh.texcoords[(i * 3 + 2) * 2 + 0] = 0.0f;
        mesh.texcoords[(i * 3 + 2) * 2 + 1] = 0.0f;
    }

    UploadMesh(&mesh, false);
    return mesh;
}
