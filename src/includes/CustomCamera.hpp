#pragma once
#include "raylib/raylib.h"

class CustomCamera {
public:
    Camera3D camera;
    float yaw;         // Yaw angle (left/right rotation) in radians
    float pitch;       // Pitch angle (up/down rotation) in radians
    float sensitivity; // Mouse look sensitivity
    float moveSpeed;   // Keyboard WASD move speed
    float zoomSpeed;   // Scroll zoom speed (changes FOV)

    CustomCamera(Vector3 position = {0.0f, 0.0f, 5.0f});

    void Update(float deltaTime);
    void Gui();
};
