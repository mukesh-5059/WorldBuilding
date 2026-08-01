#include "WorldGenerator.hpp"
#include "utils.hpp"
#include "fastnoise/FaseNoise.h"
#include "raylib/raymath.h"
#include "ConsoleLog.hpp"

#include <cmath>
#include <queue>
#include <chrono>

void Builder::RunTectonicPlateAssignment() {
    int N = cubemapFaceRes;
    int totalCells = 6 * N * N;
    if (totalCells <= 0) return;

    plates.clear();
    plates.reserve(numPlates);

    auto tStartVel = std::chrono::high_resolution_clock::now();

    for (int p = 0; p < numPlates; ++p) {
        PlateType pType = (((float)p < (float)landToWaterRatio * numPlates / 10.0f)) ? PlateType::CONTINENTAL : PlateType::OCEANIC;
        Color pCol = GetPlateColor(pType);

        // Calculate random growth step weight for plate size diversity
        float hBias = HashCell3D(Vector3{ (float)p, 9.87f, 6.54f });
        float gBias = 1.0f + (hBias * 2.0f - 1.0f) * plateSizeVariance;
        if (gBias < 0.2f) gBias = 0.2f;

        // Calculate 3D Euler Rotation Pole and Angular Speed for tectonic plate kinematics
        float eh1 = HashCell3D(Vector3{ (float)p, 5.43f, 2.10f }) * 2.0f - 1.0f;
        float eh2 = HashCell3D(Vector3{ 1.09f, (float)p, 8.76f }) * (2.0f * PI);
        float er = sqrtf(1.0f - eh1 * eh1);
        Vector3 ePole = Vector3Normalize(Vector3{ er * cosf(eh2), eh1, er * sinf(eh2) });
        float aSpeed = 0.02f + HashCell3D(Vector3{ (float)p, 3.33f, 7.77f }) * 0.06f;

        plates.push_back(TectonicPlate{ p, pType, pCol, gBias, ePole, aSpeed });
    }

    auto tEndVel = std::chrono::high_resolution_clock::now();
    double msVel = std::chrono::duration<double, std::milli>(tEndVel - tStartVel).count();
    ConsoleLog::Get().AddLog(LogLevel::Info, "[Perf] Plate Velocity Assignment: %.2f ms", msVel);

    auto tStartPlate = std::chrono::high_resolution_clock::now();

    cellPlateOwner.assign(totalCells, -1);
    cellIsLand.assign(totalCells, false);
    cellIsSeed.assign(totalCells, false);
    std::vector<float> dist(totalCells, 1e9f);

    std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> pq;

    // Pick seeds using random 3D direction vectors on the unit sphere
    for (int p = 0; p < numPlates; ++p) {
        float h1 = HashCell3D(Vector3{ (float)p, 1.23f, 4.56f }) * 2.0f - 1.0f;
        float h2 = HashCell3D(Vector3{ 7.89f, (float)p, 0.12f }) * (2.0f * PI);
        float r = sqrtf(1.0f - h1 * h1);
        Vector3 seedDir = Vector3{ r * cosf(h2), h1, r * sinf(h2) };

        // Find cell ID for seedDir
        float ax = fabsf(seedDir.x), ay = fabsf(seedDir.y), az = fabsf(seedDir.z);
        int face = 0; float uFace = 0.0f, vFace = 0.0f;
        if (ax >= ay && ax >= az) {
            if (seedDir.x > 0.0f) { face = 0; uFace = -seedDir.z / seedDir.x; vFace = seedDir.y / seedDir.x; }
            else                  { face = 1; uFace =  seedDir.z / -seedDir.x; vFace = seedDir.y / -seedDir.x; }
        } else if (ay >= ax && ay >= az) {
            if (seedDir.y > 0.0f) { face = 2; uFace =  seedDir.x / seedDir.y; vFace = -seedDir.z / seedDir.y; }
            else                  { face = 3; uFace =  seedDir.x / -seedDir.y; vFace =  seedDir.z / -seedDir.y; }
        } else {
            if (seedDir.z > 0.0f) { face = 4; uFace =  seedDir.x / seedDir.z; vFace = seedDir.y / seedDir.z; }
            else                  { face = 5; uFace = -seedDir.x / -seedDir.z; vFace = seedDir.y / -seedDir.z; }
        }
        int i = (int)(((uFace + 1.0f) * 0.5f) * (float)N);
        int j = (int)(((vFace + 1.0f) * 0.5f) * (float)N);
        if (i < 0) i = 0; if (i >= N) i = N - 1;
        if (j < 0) j = 0; if (j >= N) j = N - 1;

        int seedCell = face * N * N + i * N + j;
        dist[seedCell] = 0.0f;
        cellPlateOwner[seedCell] = p;
        cellIsSeed[seedCell] = true;

        // Expand seed marker to 4 direct neighbors so red dots are clearly visible on high-res textures
        for (int dir = 0; dir < 4; ++dir) {
            int nIdx = GetCubemapNeighborCellIndex(face, i, j, dir, N);
            if (nIdx >= 0 && nIdx < totalCells) {
                cellIsSeed[nIdx] = true;
            }
        }

        pq.push(PQElement{ 0.0f, seedCell, p });
    }

    // Seamless 3D Jittered Priority Propagation with Plate Expansion Weighting
    maxPlateDist.assign(numPlates, 0.001f);
    while (!pq.empty()) {
        PQElement current = pq.top();
        pq.pop();

        if (current.dist > dist[current.cellIndex]) continue;

        int face = current.cellIndex / (N * N);
        int rem = current.cellIndex % (N * N);
        int i = rem / N;
        int j = rem % N;

        float pBias = plates[current.plateId].growthBias;

        for (int dir = 0; dir < 4; ++dir) {
            int nIdx = GetCubemapNeighborCellIndex(face, i, j, dir, N);
            if (nIdx >= 0 && nIdx < totalCells) {
                Vector3 nPos = GetCell3DVector(nIdx, N);
                float stepNoise = (0.85f + HashCell3D(nPos) * borderJitterStrength) * pBias;
                float newDist = current.dist + stepNoise;

                if (newDist < dist[nIdx]) {
                    if (newDist > maxPlateDist[current.plateId])
                        maxPlateDist[current.plateId] = newDist;
                    dist[nIdx] = newDist;
                    cellPlateOwner[nIdx] = current.plateId;
                    pq.push(PQElement{ newDist, nIdx, current.plateId });
                }
            }
        }
    }

    auto tEndPlate = std::chrono::high_resolution_clock::now();
    double msPlate = std::chrono::duration<double, std::milli>(tEndPlate - tStartPlate).count();
    ConsoleLog::Get().AddLog(LogLevel::Info, "[Perf] Plate Assignment: %.2f ms", msPlate);

    // FastNoise 3D Continental Landmass Generation
    auto tStartNoise = std::chrono::high_resolution_clock::now();

    FastNoiseLite noise;
    noise.SetSeed(noiseSeed);
    noise.SetNoiseType((FastNoiseLite::NoiseType)noiseType);
    noise.SetFractalType((FastNoiseLite::FractalType)noiseFractalType);
    noise.SetFractalOctaves(noiseOctaves);
    noise.SetFrequency(noiseFrequency);
    noise.SetFractalLacunarity(noiseLacunarity);
    noise.SetFractalGain(noiseGain);

    cellIsLand.assign(totalCells, false);

    for (int c = 0; c < totalCells; ++c) {
        int p = cellPlateOwner[c];
        if (p >= 0 && p < numPlates) {
            if (plates[p].type == PlateType::CONTINENTAL) {
                Vector3 pos = GetCell3DVector(c, N);
                // Sample 3D noise
                float noiseVal = noise.GetNoise(pos.x, pos.y, pos.z); // [-1.0, 1.0]

                // Distance from plate seed normalized
                float normDist = (maxPlateDist[p] > 0.001f) ? (dist[c] / maxPlateDist[p]) : 1.0f;

                // Combine distance falloff with 3D noise for organic continent shape
                float landValue = (1.0f - normDist) * 1.4f + (noiseVal * noiseStrength);
                cellIsLand[c] = (landValue > seaLevel);
            } else {
                cellIsLand[c] = false;
            }
        }
    }

    auto tEndNoise = std::chrono::high_resolution_clock::now();
    double msNoise = std::chrono::duration<double, std::milli>(tEndNoise - tStartNoise).count();
    ConsoleLog::Get().AddLog(LogLevel::Info, "[Perf] 3D Noise Generation: %.2f ms", msNoise);

    cellPlateDist = dist;
}