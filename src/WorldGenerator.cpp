#include "includes/WorldGenerator.hpp"
#include "raylib/raymath.h"
#include <vector>
#include <map>
#include <queue>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <cfloat>
#include <thread>

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

World GenerateWorld(int subdivisions, float radius, int numPlates) {
    World world;
    world.radius = radius;
    world.subdivisions = subdivisions;
    world.numPlates = numPlates;
    world.generated = false;

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

    int tileCount = (int)faces.size();
    world.tiles.resize(tileCount);

    for (int i = 0; i < tileCount; ++i) {
        world.tiles[i].id = i;
        world.tiles[i].corners[0] = vertices[faces[i].v1];
        world.tiles[i].corners[1] = vertices[faces[i].v2];
        world.tiles[i].corners[2] = vertices[faces[i].v3];
        world.tiles[i].center = Vector3Scale(Vector3Add(Vector3Add(world.tiles[i].corners[0], world.tiles[i].corners[1]), world.tiles[i].corners[2]), 1.0f / 3.0f);
        world.tiles[i].plateId = -1;
        world.tiles[i].neighbors[0] = -1;
        world.tiles[i].neighbors[1] = -1;
        world.tiles[i].neighbors[2] = -1;
    }

    std::map<std::pair<int, int>, std::vector<int>> edgeToTiles;
    for (int i = 0; i < tileCount; ++i) {
        int v1 = faces[i].v1;
        int v2 = faces[i].v2;
        int v3 = faces[i].v3;
        edgeToTiles[std::make_pair(std::min(v1, v2), std::max(v1, v2))].push_back(i);
        edgeToTiles[std::make_pair(std::min(v2, v3), std::max(v2, v3))].push_back(i);
        edgeToTiles[std::make_pair(std::min(v3, v1), std::max(v3, v1))].push_back(i);
    }

    for (int i = 0; i < tileCount; ++i) {
        int v1 = faces[i].v1;
        int v2 = faces[i].v2;
        int v3 = faces[i].v3;
        std::pair<int, int> edges[3] = {
            std::make_pair(std::min(v1, v2), std::max(v1, v2)),
            std::make_pair(std::min(v2, v3), std::max(v2, v3)),
            std::make_pair(std::min(v3, v1), std::max(v3, v1))
        };
        for (int e = 0; e < 3; ++e) {
            const auto& shared = edgeToTiles[edges[e]];
            for (int neighborId : shared) {
                if (neighborId != i) {
                    world.tiles[i].neighbors[e] = neighborId;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < numPlates; ++i) {
        unsigned char r = (unsigned char)GetRandomValue(40, 220);
        unsigned char g = (unsigned char)GetRandomValue(40, 220);
        unsigned char b = (unsigned char)GetRandomValue(40, 220);
        world.plateColors.push_back({ r, g, b, 255 });
    }

    std::queue<int> openList;
    std::vector<int> indices(tileCount);
    for (int i = 0; i < tileCount; ++i) indices[i] = i;
    for (int i = tileCount - 1; i > 0; --i) {
        int j = GetRandomValue(0, i);
        std::swap(indices[i], indices[j]);
    }

    int seedsPlaced = 0;
    for (int idx : indices) {
        if (seedsPlaced >= numPlates) break;
        world.tiles[idx].plateId = seedsPlaced;
        openList.push(idx);
        seedsPlaced++;
    }

    while (!openList.empty()) {
        int curr = openList.front();
        openList.pop();
        int currentPlate = world.tiles[curr].plateId;

        for (int n = 0; n < 3; ++n) {
            int neighborId = world.tiles[curr].neighbors[n];
            if (neighborId != -1 && world.tiles[neighborId].plateId == -1) {
                world.tiles[neighborId].plateId = currentPlate;
                openList.push(neighborId);
            }
        }
    }

    return world;
}

void RebuildWorldModel(World& world, Shader shader) {
    if (world.generated) {
        UnloadModel(world.model);
    }

    Mesh mesh = { 0 };
    mesh.vertexCount = (int)world.tiles.size() * 3;
    mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount * 2 * sizeof(float));
    mesh.normals = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    mesh.colors = (unsigned char *)RL_MALLOC(mesh.vertexCount * 4 * sizeof(unsigned char));

    for (size_t i = 0; i < world.tiles.size(); ++i) {
        const auto& tile = world.tiles[i];
        Vector3 v1 = tile.corners[0];
        Vector3 v2 = tile.corners[1];
        Vector3 v3 = tile.corners[2];

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

        for (int j = 0; j < 3; ++j) {
            mesh.vertices[(i * 3 + j) * 3 + 0] = tile.corners[j].x * world.radius;
            mesh.vertices[(i * 3 + j) * 3 + 1] = tile.corners[j].y * world.radius;
            mesh.vertices[(i * 3 + j) * 3 + 2] = tile.corners[j].z * world.radius;

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

        Color col = (tile.plateId != -1) ? world.plateColors[tile.plateId] : WHITE;
        for (int j = 0; j < 3; ++j) {
            mesh.colors[(i * 3 + j) * 4 + 0] = col.r;
            mesh.colors[(i * 3 + j) * 4 + 1] = col.g;
            mesh.colors[(i * 3 + j) * 4 + 2] = col.b;
            mesh.colors[(i * 3 + j) * 4 + 3] = col.a;
        }
    }

    UploadMesh(&mesh, false);
    world.mesh = mesh;
    world.model = LoadModelFromMesh(mesh);
    world.model.materials[0].shader = shader;
    world.generated = true;
}

void UnloadWorld(World& world) {
    if (world.generated) {
        UnloadModel(world.model);
        world.generated = false;
    }
}

static void RunExportThread(std::vector<WorldTile> tiles, std::vector<Color> plateColors, std::string filepath, int width, int height, std::atomic<float>* progress, std::atomic<bool>* running) {
    std::filesystem::path path(filepath);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    Image image = GenImageColor(width, height, BLANK);
    Color* pixels = (Color*)image.data;
    for (int y = 0; y < height; ++y) {
        float phi = ((float)y / (float)height) * PI;
        float sinPhi = sinf(phi);
        float cosPhi = cosf(phi);
        for (int x = 0; x < width; ++x) {
            float theta = ((float)x / (float)width) * 2.0f * PI;
            Vector3 d;
            d.x = sinPhi * sinf(theta);
            d.y = cosPhi;
            d.z = sinPhi * cosf(theta);
            int bestTile = 0;
            float maxDot = -FLT_MAX;
            for (size_t i = 0; i < tiles.size(); ++i) {
                float dot = tiles[i].center.x * d.x + 
                            tiles[i].center.y * d.y + 
                            tiles[i].center.z * d.z;
                if (dot > maxDot) {
                    maxDot = dot;
                    bestTile = (int)i;
                }
            }
            int plateId = tiles[bestTile].plateId;
            pixels[y * width + x] = (plateId != -1) ? plateColors[plateId] : WHITE;
        }
        progress->store((float)(y + 1) / (float)height);
    }
    ExportImage(image, filepath.c_str());
    UnloadImage(image);
    running->store(false);
}

void ExportWorldMapAsync(const std::vector<WorldTile>& tiles, const std::vector<Color>& plateColors, const std::string& filepath, int width, int height, std::atomic<float>& progress, std::atomic<bool>& running) {
    running.store(true);
    progress.store(0.0f);
    std::thread t(RunExportThread, tiles, plateColors, filepath, width, height, &progress, &running);
    t.detach();
}
