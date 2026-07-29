#include "includes/WorldGenerator.hpp"
#include "includes/utils.hpp"
#include "raylib/raymath.h"

#include <cmath>
#include <queue>

// Returns the neighbor cell index across 12 cubemap face edges
static int GetCubemapNeighborCellIndex(int face, int i, int j, int dir, int N) {
    int ni = i, nj = j, nFace = face;

    if (dir == 0) ni = i - 1;
    else if (dir == 1) ni = i + 1;
    else if (dir == 2) nj = j - 1;
    else if (dir == 3) nj = j + 1;

    if (ni >= 0 && ni < N && nj >= 0 && nj < N) {
        return nFace * N * N + ni * N + nj;
    }

    // Cross-Face Boundary Neighbor mapping across 12 cube edges
    if (face == 0) { // +X
        if (ni < 0)  { nFace = 4; ni = N - 1; nj = j; }
        if (ni >= N) { nFace = 5; ni = 0;     nj = j; }
        if (nj < 0)  { nFace = 3; ni = N - 1; nj = N - 1 - i; }
        if (nj >= N) { nFace = 2; ni = N - 1; nj = i; }
    } else if (face == 1) { // -X
        if (ni < 0)  { nFace = 5; ni = N - 1; nj = j; }
        if (ni >= N) { nFace = 4; ni = 0;     nj = j; }
        if (nj < 0)  { nFace = 3; ni = 0;     nj = i; }
        if (nj >= N) { nFace = 2; ni = 0;     nj = N - 1 - i; }
    } else if (face == 2) { // +Y
        if (ni < 0)  { nFace = 1; ni = N - 1 - j; nj = N - 1; }
        if (ni >= N) { nFace = 0; ni = j;         nj = N - 1; }
        if (nj < 0)  { nFace = 4; ni = i;         nj = N - 1; }
        if (nj >= N) { nFace = 5; ni = N - 1 - i; nj = N - 1; }
    } else if (face == 3) { // -Y
        if (ni < 0)  { nFace = 1; ni = j;         nj = 0; }
        if (ni >= N) { nFace = 0; ni = N - 1 - j; nj = 0; }
        if (nj < 0)  { nFace = 5; ni = N - 1 - i; nj = 0; }
        if (nj >= N) { nFace = 4; ni = i;         nj = 0; }
    } else if (face == 4) { // +Z
        if (ni < 0)  { nFace = 1; ni = N - 1; nj = j; }
        if (ni >= N) { nFace = 0; ni = 0;     nj = j; }
        if (nj < 0)  { nFace = 3; ni = i;     nj = N - 1; }
        if (nj >= N) { nFace = 2; ni = i;     nj = 0; }
    } else if (face == 5) { // -Z
        if (ni < 0)  { nFace = 0; ni = N - 1; nj = j; }
        if (ni >= N) { nFace = 1; ni = 0;     nj = j; }
        if (nj < 0)  { nFace = 3; ni = N - 1 - i; nj = 0; }
        if (nj >= N) { nFace = 2; ni = N - 1 - i; nj = N - 1; }
    }

    return nFace * N * N + ni * N + nj;
}

static Vector3 GetCell3DVector(int cellIndex, int N) {
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


static float HashCell3D(Vector3 p) {
    unsigned int x = (unsigned int)(p.x * 73856093.0f);
    unsigned int y = (unsigned int)(p.y * 19349663.0f);
    unsigned int z = (unsigned int)(p.z * 83492791.0f);
    unsigned int n = x ^ (y * 31u) ^ (z * 57u);
    n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return ((float)(n & 0xFFFF) / 65535.0f);
}

struct PQElement {
    float dist;
    int cellIndex;
    int plateId;

    bool operator>(const PQElement& other) const {
        return dist > other.dist;
    }
};


void Builder::RunTectonicPlateAssignment() {
    int N = cubemapFaceRes;
    int totalCells = 6 * N * N;
    if (totalCells <= 0) return;


    plates.clear();
    plates.reserve(numPlates);


    for (int p = 0; p < numPlates; ++p) {
        PlateType pType = (((float)p < (float)landToWaterRatio * numPlates / 10.0f)) ? PlateType::CONTINENTAL : PlateType::OCEANIC;
        Color pCol = GetPlateColor(pType);


        // Calculate random growth step weight for plate size diversity
        float hBias = HashCell3D(Vector3{ (float)p, 9.87f, 6.54f }); // Range 0.0 to 1.0
        float gBias = 1.0f + (hBias * 2.0f - 1.0f) * plateSizeVariance;
        if (gBias < 0.2f) gBias = 0.2f;


        plates.push_back(TectonicPlate{ p, pType, pCol, gBias });
    }


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


    // Determine final Land vs Water state using Land Bias Multiplier
    for (int c = 0; c < totalCells; ++c) {
        int p = cellPlateOwner[c];
        if (p >= 0 && p < numPlates) {
            if (plates[p].type == PlateType::CONTINENTAL) {
                float normDist = (dist[c] / maxPlateDist[p]) * landBiasMultiplier;
                cellIsLand[c] = (normDist <= 1.0f);
            } else {
                cellIsLand[c] = false;
            }
        }
    }


    cellPlateDist = dist;
}