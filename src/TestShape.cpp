#include "includes/TestShape.hpp"
#include <cmath>

Mesh CreateTestPyramid() {
    Mesh mesh = { 0 };
    mesh.vertexCount = 18;
    mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount * 2 * sizeof(float));
    mesh.normals = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));

    Vector3 tip = { 0.0f, 1.0f, 0.0f };
    Vector3 v00 = { -1.0f, 0.0f, -1.0f };
    Vector3 v10 = {  1.0f, 0.0f, -1.0f };
    Vector3 v11 = {  1.0f, 0.0f,  1.0f };
    Vector3 v01 = { -1.0f, 0.0f,  1.0f };

    Vector3 triangles[18] = {
        tip, v10, v00,
        tip, v11, v10,
        tip, v01, v11,
        tip, v00, v01,
        v00, v10, v11,
        v00, v11, v01
    };

    for (int i = 0; i < 6; i++) {
        Vector3 a = triangles[i * 3 + 0];
        Vector3 b = triangles[i * 3 + 1];
        Vector3 c = triangles[i * 3 + 2];

        Vector3 edge1 = { b.x - a.x, b.y - a.y, b.z - a.z };
        Vector3 edge2 = { c.x - a.x, c.y - a.y, c.z - a.z };
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

        for (int j = 0; j < 3; j++) {
            mesh.vertices[(i * 3 + j) * 3 + 0] = triangles[i * 3 + j].x;
            mesh.vertices[(i * 3 + j) * 3 + 1] = triangles[i * 3 + j].y;
            mesh.vertices[(i * 3 + j) * 3 + 2] = triangles[i * 3 + j].z;

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
