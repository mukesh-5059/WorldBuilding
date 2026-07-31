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

Color GetPlateColor(PlateType type) {
    if (type == PlateType::OCEANIC) return Color{ 35, 85, 205, 255 }; //Royal Blue 
    else return Color{ 45, 185, 45, 255 }; //Vibrant Green
}

float HashCell3D(Vector3 p) {
    unsigned int x = (unsigned int)(p.x * 73856093.0f);
    unsigned int y = (unsigned int)(p.y * 19349663.0f);
    unsigned int z = (unsigned int)(p.z * 83492791.0f);
    unsigned int n = x ^ (y * 31u) ^ (z * 57u);
    n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return ((float)(n & 0xFFFF) / 65535.0f);
}

Vector3 GetCell3DVector(int cellIndex, int N) {
    int face = cellIndex / (N * N);
    int rem = cellIndex % (N * N);
    int i = rem / N;
    int j = rem % N;

    float u0 = -1.0f + ((float)i + 0.5f) * (2.0f / (float)N);
    float v0 = -1.0f + ((float)j + 0.5f) * (2.0f / (float)N);

    Vector3 cubeP = { 0.0f, 0.0f, 0.0f };
    switch (face) {
        case 0: cubeP = Vector3{ 1.0f,  v0, -u0 }; break;
        case 1: cubeP = Vector3{-1.0f,  v0,  u0 }; break;
        case 2: cubeP = Vector3{ u0,  1.0f, -v0 }; break;
        case 3: cubeP = Vector3{ u0, -1.0f,  v0 }; break;
        case 4: cubeP = Vector3{ u0,  v0,  1.0f }; break;
        case 5: cubeP = Vector3{-u0,  v0, -1.0f }; break;
    }

    return Vector3Normalize(cubeP);
}

int GetCellIdFrom3DVector(Vector3 dir, int cubemapFaceRes) {
    float ax = fabsf(dir.x), ay = fabsf(dir.y), az = fabsf(dir.z);
    int face = 0; float uFace = 0.0f, vFace = 0.0f;
    if (ax >= ay && ax >= az) {
        if (dir.x > 0.0f) { face = 0; uFace = -dir.z / dir.x; vFace = dir.y / dir.x; }
        else             { face = 1; uFace =  dir.z / -dir.x; vFace = dir.y / -dir.x; }
    } else if (ay >= ax && ay >= az) {
        if (dir.y > 0.0f) { face = 2; uFace =  dir.x / dir.y; vFace = -dir.z / dir.y; }
        else             { face = 3; uFace =  dir.x / -dir.y; vFace =  dir.z / -dir.y; }
    } else {
        if (dir.z > 0.0f) { face = 4; uFace =  dir.x / dir.z; vFace = dir.y / dir.z; }
        else             { face = 5; uFace = -dir.x / -dir.z; vFace = dir.y / -dir.z; }
    }
    int i = (int)(((uFace + 1.0f) * 0.5f) * (float)cubemapFaceRes);
    int j = (int)(((vFace + 1.0f) * 0.5f) * (float)cubemapFaceRes);
    if (i < 0) i = 0; if (i >= cubemapFaceRes) i = cubemapFaceRes - 1;
    if (j < 0) j = 0; if (j >= cubemapFaceRes) j = cubemapFaceRes - 1;

    return face * cubemapFaceRes * cubemapFaceRes + i * cubemapFaceRes + j;
}

int GetCubemapNeighborCellIndex(int face, int i, int j, int dir, int N) {
    int ni = i, nj = j, nFace = face;

    if (dir == 0) ni = i - 1;
    else if (dir == 1) ni = i + 1;
    else if (dir == 2) nj = j - 1;
    else if (dir == 3) nj = j + 1;

    if (ni >= 0 && ni < N && nj >= 0 && nj < N) {
        return nFace * N * N + ni * N + nj;
    }

    if (face == 0) {
        if (ni < 0)  { nFace = 4; ni = N - 1; nj = j; }
        if (ni >= N) { nFace = 5; ni = 0;     nj = j; }
        if (nj < 0)  { nFace = 3; ni = N - 1; nj = N - 1 - i; }
        if (nj >= N) { nFace = 2; ni = N - 1; nj = i; }
    } else if (face == 1) {
        if (ni < 0)  { nFace = 5; ni = N - 1; nj = j; }
        if (ni >= N) { nFace = 4; ni = 0;     nj = j; }
        if (nj < 0)  { nFace = 3; ni = 0;     nj = i; }
        if (nj >= N) { nFace = 2; ni = 0;     nj = N - 1 - i; }
    } else if (face == 2) {
        if (ni < 0)  { nFace = 1; ni = N - 1 - j; nj = N - 1; }
        if (ni >= N) { nFace = 0; ni = j;         nj = N - 1; }
        if (nj < 0)  { nFace = 4; ni = i;         nj = N - 1; }
        if (nj >= N) { nFace = 5; ni = N - 1 - i; nj = N - 1; }
    } else if (face == 3) {
        if (ni < 0)  { nFace = 1; ni = j;         nj = 0; }
        if (ni >= N) { nFace = 0; ni = N - 1 - j; nj = 0; }
        if (nj < 0)  { nFace = 5; ni = N - 1 - i; nj = 0; }
        if (nj >= N) { nFace = 4; ni = i;         nj = 0; }
    } else if (face == 4) {
        if (ni < 0)  { nFace = 1; ni = N - 1; nj = j; }
        if (ni >= N) { nFace = 0; ni = 0;     nj = j; }
        if (nj < 0)  { nFace = 3; ni = i;     nj = N - 1; }
        if (nj >= N) { nFace = 2; ni = i;     nj = 0; }
    } else if (face == 5) {
        if (ni < 0)  { nFace = 0; ni = N - 1; nj = j; }
        if (ni >= N) { nFace = 1; ni = 0;     nj = j; }
        if (nj < 0)  { nFace = 3; ni = N - 1 - i; nj = 0; }
        if (nj >= N) { nFace = 2; ni = N - 1 - i; nj = N - 1; }
    }

    return nFace * N * N + ni * N + nj;
}