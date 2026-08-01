#include "WorldGenerator.hpp"
#include "utils.hpp"
#include "fastnoise/FaseNoise.h"
#include "ConsoleLog.hpp"
#include <chrono>
#include <cmath>

void Builder::GenerateBoundaryIslands() {
    if (!enableIslands || boundaryCells.empty() || cellPlateOwner.empty()) return;

    auto tStartIslands = std::chrono::high_resolution_clock::now();

    int N = cubemapFaceRes;
    int totalCells = 6 * N * N;

    // High-frequency FastNoise instance for Island Arc & Ridge Archipelagos
    FastNoiseLite islandNoise;
    islandNoise.SetSeed(worldSeed + 9999);
    islandNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    islandNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    islandNoise.SetFractalOctaves(3);
    islandNoise.SetFrequency(12.0f * islandDensity); // Frequency scales island archipelago cluster spacing

    for (const auto& bData : boundaryCells) {
        int cellId = bData.cellIndex;
        int pA = bData.plateA;
        int pB = bData.plateB;

        if (pA < 0 || pA >= (int)plates.size() || pB < 0 || pB >= (int)plates.size()) continue;

        // Island Formation Criteria:
        // 1. Oceanic vs Oceanic Convergent (Subduction Island Arcs e.g. Japan, Philippines, Aleutians)
        // 2. Oceanic vs Oceanic Divergent (Spreading Ridge Islands e.g. Iceland, Azores)
        bool isOceanicA = (plates[pA].type == PlateType::OCEANIC);
        bool isOceanicB = (plates[pB].type == PlateType::OCEANIC);

        if (isOceanicA && isOceanicB) {
            BoundaryType bType = bData.type;
            if (bType == BoundaryType::CONVERGENT || bType == BoundaryType::DIVERGENT) {
                Vector3 pos = cell3DVectorMap[cellId];

                // Sample high-frequency 3D noise for discrete island peaks
                float noiseVal = islandNoise.GetNoise(pos.x, pos.y, pos.z); // [-1.0, 1.0]
                float normalizedNoise = (noiseVal + 1.0f) * 0.5f;          // [0.0, 1.0]

                // Compression magnitude boosts volcanic mountain height
                float compStrength = fabsf(bData.compressionRate);
                float islandElevation = normalizedNoise * (0.5f + compStrength * 20.0f) * islandSizeScale;

                if (islandElevation > (seaLevel * 0.85f)) {
                    cellIsLand[cellId] = true;

                    // Expand island width to adjacent neighbors based on islandSizeScale
                    int face = cellId / (N * N);
                    int rem = cellId % (N * N);
                    int i = rem / N;
                    int j = rem % N;

                    for (int dir = 0; dir < 4; ++dir) {
                        int nIdx = GetCubemapNeighborCellIndex(face, i, j, dir, N);
                        if (nIdx >= 0 && nIdx < totalCells) {
                            Vector3 nPos = cell3DVectorMap[nIdx];
                            float nNoise = (islandNoise.GetNoise(nPos.x, nPos.y, nPos.z) + 1.0f) * 0.5f;
                            if (nNoise > (0.45f / islandSizeScale)) {
                                cellIsLand[nIdx] = true;
                            }
                        }
                    }
                }
            }
        }
    }

    auto tEndIslands = std::chrono::high_resolution_clock::now();
    double msIslands = std::chrono::duration<double, std::milli>(tEndIslands - tStartIslands).count();
    ConsoleLog::Get().AddLog(LogLevel::Performance, "[Perf] Boundary Island Generation: %.2f ms", msIslands);
}
