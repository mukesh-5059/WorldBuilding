#include "includes/CustomCamera.hpp"
#include "raylib/raymath.h"
#include "imgui/imgui.h"
#include <cmath>

CustomCamera::CustomCamera(Vector3 position) {
    camera.position = position;
    camera.target = Vector3Add(position, Vector3{ 0.0f, 0.0f, -1.0f });
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    yaw = PI;
    pitch = 0.0f;
    sensitivity = 0.003f;
    moveSpeed = 10.0f;
    zoomSpeed = 5.0f;
}

void CustomCamera::Update(float deltaTime) {
    bool wantCaptureMouse = ImGui::GetIO().WantCaptureMouse;
    bool wantCaptureKeyboard = ImGui::GetIO().WantCaptureKeyboard;

    if (!wantCaptureMouse) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            camera.fovy -= wheel * zoomSpeed;
            if (camera.fovy < 10.0f) camera.fovy = 10.0f;
            if (camera.fovy > 120.0f) camera.fovy = 120.0f;
        }
    }

    if (!wantCaptureMouse) {
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 delta = GetMouseDelta();
            yaw -= delta.x * sensitivity;
            pitch -= delta.y * sensitivity;

            const float maxPitch = 89.0f * (PI / 180.0f);
            if (pitch > maxPitch) pitch = maxPitch;
            if (pitch < -maxPitch) pitch = -maxPitch;
        }
    }

    Vector3 forward;
    forward.x = sinf(yaw) * cosf(pitch);
    forward.y = sinf(pitch);
    forward.z = cosf(yaw) * cosf(pitch);

    Vector3 right;
    right.x = cosf(yaw);
    right.y = 0.0f;
    right.z = -sinf(yaw);

    if (!wantCaptureKeyboard) {
        Vector3 movement = { 0.0f, 0.0f, 0.0f };

        if (IsKeyDown(KEY_W)) movement = Vector3Add(movement, forward);
        if (IsKeyDown(KEY_S)) movement = Vector3Subtract(movement, forward);
        if (IsKeyDown(KEY_A)) movement = Vector3Add(movement, right);
        if (IsKeyDown(KEY_D)) movement = Vector3Subtract(movement, right);

        if (IsKeyDown(KEY_SPACE)) movement.y += 1.0f;
        if (IsKeyDown(KEY_LEFT_SHIFT)) movement.y -= 1.0f;

        if (Vector3LengthSqr(movement) > 0.0f) {
            movement = Vector3Normalize(movement);
            movement = Vector3Scale(movement, moveSpeed * deltaTime);
            camera.position = Vector3Add(camera.position, movement);
        }
    }

    camera.target = Vector3Add(camera.position, forward);
}
void CustomCamera::Gui(){
    ImGui::SliderFloat("Movement Speed", &moveSpeed, 1.0f, 50.0f, "%.1f");
    ImGui::SliderFloat("Look Sensitivity", &sensitivity, 0.0005f, 0.01f, "%.5f");
    ImGui::SliderFloat("Zoom Speed", &zoomSpeed, 1.0f, 20.0f, "%.1f");
}