#include "WorldGenerator.hpp"
#include "utils.hpp"
#include "raylib/raymath.h"
#include <vector>

void Builder::EvaluateTectonicBoundaryCollisions() {
    int N = cubemapFaceRes;
    int totalCells = 6 * N * N;
    if (totalCells <= 0 || cellPlateOwner.empty() || plates.empty()) return;

    cellBoundaryType.assign(totalCells, BoundaryType::NONE);
    cellCompressionRate.assign(totalCells, 0.0f);
    boundaryCells.clear();

    for (int c = 0; c < totalCells; ++c) {
        int plateA = cellPlateOwner[c];
        if (plateA < 0 || plateA >= (int)plates.size()) continue;

        int face = c / (N * N);
        int rem = c % (N * N);
        int i = rem / N;
        int j = rem % N;

        Vector3 posA = GetCell3DVector(c, N);
        const auto& pA = plates[plateA];
        Vector3 vA = Vector3CrossProduct(posA, Vector3Scale(pA.eulerPole, pA.angularSpeed));

        for (int dir = 0; dir < 4; ++dir) {
            int nIdx = GetCubemapNeighborCellIndex(face, i, j, dir, N);
            if (nIdx < 0 || nIdx >= totalCells) continue;

            int plateB = cellPlateOwner[nIdx];
            if (plateB != plateA && plateB >= 0 && plateB < (int)plates.size()) {
                Vector3 posB = GetCell3DVector(nIdx, N);
                const auto& pB = plates[plateB];
                Vector3 vB = Vector3CrossProduct(posA, Vector3Scale(pB.eulerPole, pB.angularSpeed));

                // Relative velocity of Plate A with respect to Plate B at boundary posA: V_rel = V_A - V_B
                Vector3 vRel = Vector3Subtract(vA, vB);

                // Border normal vector pointing outward from cell A into cell B
                Vector3 nBorder = Vector3Normalize(Vector3Subtract(posB, posA));

                // Compression rate: K = V_rel . N_border
                // Positive K = Convergent (V_rel points into Plate B -> Slamming together)
                // Negative K = Divergent (V_rel points away from Plate B -> Pulling apart)
                float compression = Vector3DotProduct(vRel, nBorder);

                BoundaryType bType = BoundaryType::TRANSFORM;
                float threshold = 0.003f;

                if (compression > threshold) {
                    bType = BoundaryType::CONVERGENT; // Red
                } else if (compression < -threshold) {
                    bType = BoundaryType::DIVERGENT;  // Cyan
                } else {
                    bType = BoundaryType::TRANSFORM;  // Orange
                }

                // Assign boundary type to BOTH contacting cells across the boundary
                cellBoundaryType[c] = bType;
                cellBoundaryType[nIdx] = bType;

                cellCompressionRate[c] = compression;

                boundaryCells.push_back(BoundaryCellData{ c, plateA, plateB, bType, compression, vRel });
                break;
            }
        }
    }
}
