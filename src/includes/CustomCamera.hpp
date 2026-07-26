#pragma once
#include "raylib/raylib.h"

enum class CameraType {
    FreeFly = 0,
    Orbit = 1
};

class CustomCamera {
public:
    Camera3D camera;
    CameraType mode;

    float yaw;         // Yaw angle (left/right rotation) in radians
    float pitch;       // Pitch angle (up/down rotation) in radians
    float sensitivity; // Mouse look sensitivity
    float moveSpeed;   // Keyboard WASD move speed
    float zoomSpeed;   // Scroll zoom speed

    // Orbit / Object Inspector Mode Properties
    Vector3 orbitTarget;
    float orbitDistance;

    CustomCamera(Vector3 position = {0.0f, 2.0f, 5.0f});

    void Update(float deltaTime, bool isViewportHovered = true);
    void Gui();
    void SetMode(CameraType newMode);
    void SyncOrbitFromPosition();
};
