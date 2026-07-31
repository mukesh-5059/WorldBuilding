#include "CustomCamera.hpp"
#include "raylib/raymath.h"
#include "imgui/imgui.h"
#include <cmath>

CustomCamera::CustomCamera(Vector3 position) {
    camera.position = position;
    orbitTarget = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.target = orbitTarget;
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    mode = CameraType::Orbit; // Default to Orbit (Inspect) mode

    sensitivity = 0.003f;
    moveSpeed = 10.0f;
    zoomSpeed = 2.0f;

    SyncOrbitFromPosition();
}

void CustomCamera::SyncOrbitFromPosition() {
    Vector3 dir = Vector3Subtract(camera.position, orbitTarget);
    orbitDistance = Vector3Length(dir);
    if (orbitDistance < 0.1f) orbitDistance = 0.1f;

    pitch = asinf(Clamp(dir.y / orbitDistance, -1.0f, 1.0f));
    yaw = atan2f(dir.x, dir.z);
}

void CustomCamera::SetMode(CameraType newMode) {
    if (mode == newMode) return;

    if (newMode == CameraType::Orbit) {
        SyncOrbitFromPosition();
    } else if (newMode == CameraType::FreeFly) {
        Vector3 forward;
        forward.x = sinf(yaw) * cosf(pitch);
        forward.y = sinf(pitch);
        forward.z = cosf(yaw) * cosf(pitch);
        camera.target = Vector3Add(camera.position, forward);
    }

    mode = newMode;
}

void CustomCamera::Update(float deltaTime, bool isViewportHovered) {
    bool allowInput = isViewportHovered || IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    if (mode == CameraType::Orbit) {
        if (allowInput) {
            // Zoom (adjusts distance to origin target)
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) {
                orbitDistance -= wheel * zoomSpeed;
                if (orbitDistance < 0.5f) orbitDistance = 0.5f;
                if (orbitDistance > 100.0f) orbitDistance = 100.0f;
            }

            // Mouse Orbit Rotation
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 delta = GetMouseDelta();
                yaw -= delta.x * sensitivity;
                pitch += delta.y * sensitivity;

                const float maxPitch = 89.0f * (PI / 180.0f);
                if (pitch > maxPitch) pitch = maxPitch;
                if (pitch < -maxPitch) pitch = -maxPitch;
            }
        }

        // Calculate 3D position on sphere around orbitTarget
        camera.position.x = orbitTarget.x + orbitDistance * cosf(pitch) * sinf(yaw);
        camera.position.y = orbitTarget.y + orbitDistance * sinf(pitch);
        camera.position.z = orbitTarget.z + orbitDistance * cosf(pitch) * cosf(yaw);

        camera.target = orbitTarget;
        camera.up = Vector3{ 0.0f, 1.0f, 0.0f };

    } else { // FreeFly Mode
        Vector3 forward;
        forward.x = sinf(yaw) * cosf(pitch);
        forward.y = sinf(pitch);
        forward.z = cosf(yaw) * cosf(pitch);

        Vector3 right;
        right.x = cosf(yaw);
        right.y = 0.0f;
        right.z = -sinf(yaw);

        if (allowInput) {
            // FOV Zoom
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) {
                camera.fovy -= wheel * zoomSpeed * 2.0f;
                if (camera.fovy < 10.0f) camera.fovy = 10.0f;
                if (camera.fovy > 120.0f) camera.fovy = 120.0f;
            }

            // Mouse Look Rotation
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 delta = GetMouseDelta();
                yaw -= delta.x * sensitivity;
                pitch -= delta.y * sensitivity;

                const float maxPitch = 89.0f * (PI / 180.0f);
                if (pitch > maxPitch) pitch = maxPitch;
                if (pitch < -maxPitch) pitch = -maxPitch;
            }

            // Keyboard Movement (WASD + Space + Shift)
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
        camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    }
}

void CustomCamera::Gui() {
    // 1. Camera Mode Toggle Buttons (No dropdown)
    ImGui::Text("Camera Mode:");
    bool isOrbit = (mode == CameraType::Orbit);
    bool isFreeFly = (mode == CameraType::FreeFly);

    if (isOrbit) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.55f, 0.85f, 1.0f));
    if (ImGui::Button("Orbit Mode", ImVec2(130.0f, 0.0f))) {
        SetMode(CameraType::Orbit);
    }
    if (isOrbit) ImGui::PopStyleColor();

    ImGui::SameLine();

    if (isFreeFly) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.55f, 0.85f, 1.0f));
    if (ImGui::Button("Free Fly Mode", ImVec2(130.0f, 0.0f))) {
        SetMode(CameraType::FreeFly);
    }
    if (isFreeFly) ImGui::PopStyleColor();

    ImGui::Separator();

    // 2. Camera Position Controls & Display
    float pos[3] = { camera.position.x, camera.position.y, camera.position.z };
    if (ImGui::DragFloat3("Position (XYZ)", pos, 0.1f, -100.0f, 100.0f, "%.2f")) {
        camera.position = Vector3{ pos[0], pos[1], pos[2] };
        if (mode == CameraType::Orbit) {
            SyncOrbitFromPosition();
        }
    }

    // 3. Camera Rotation Controls & Display (in Degrees)
    float yawDeg = yaw * (180.0f / PI);
    float pitchDeg = pitch * (180.0f / PI);
    bool rotChanged = false;

    if (ImGui::DragFloat("Yaw (Rotation Y)", &yawDeg, 0.5f, -360.0f, 360.0f, "%.1f deg")) rotChanged = true;
    if (ImGui::DragFloat("Pitch (Rotation X)", &pitchDeg, 0.5f, -89.0f, 89.0f, "%.1f deg")) rotChanged = true;

    if (rotChanged) {
        yaw = yawDeg * (PI / 180.0f);
        pitch = pitchDeg * (PI / 180.0f);
    }

    ImGui::Separator();

    // 4. Mode Specific Settings
    if (mode == CameraType::Orbit) {
        if (ImGui::SliderFloat("Orbit Distance", &orbitDistance, 0.5f, 50.0f, "%.2f")) {
            // Distance updated
        }
        float targetArr[3] = { orbitTarget.x, orbitTarget.y, orbitTarget.z };
        if (ImGui::DragFloat3("Target Origin", targetArr, 0.1f, -50.0f, 50.0f, "%.2f")) {
            orbitTarget = Vector3{ targetArr[0], targetArr[1], targetArr[2] };
        }
    } else {
        ImGui::SliderFloat("Movement Speed", &moveSpeed, 1.0f, 50.0f, "%.1f");
    }

    ImGui::SliderFloat("Field of View", &camera.fovy, 10.0f, 120.0f, "%.1f deg");
    ImGui::SliderFloat("Look Sensitivity", &sensitivity, 0.0005f, 0.01f, "%.5f");
    ImGui::SliderFloat("Zoom Speed", &zoomSpeed, 0.1f, 10.0f, "%.1f");

    ImGui::Spacing();
    if (ImGui::Button("Reset Camera to Default", ImVec2(-1, 0))) {
        camera.position = Vector3{ 0.0f, 2.0f, 5.0f };
        orbitTarget = Vector3{ 0.0f, 0.0f, 0.0f };
        camera.fovy = 60.0f;
        SyncOrbitFromPosition();
    }
}