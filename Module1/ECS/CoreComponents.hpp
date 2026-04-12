#include "glmcommon.hpp"
#include "RenderableMesh.hpp"
#include "InputManager.hpp"

#pragma once

using namespace eeng;

struct TransformComponent {
    glm::vec3 position;
    glm::mat3x3 rotation;
    glm::vec3 scale;
};

struct LinearVelocityComponent {
    glm::vec3 velocity;
};

struct MeshComponent {
    std::weak_ptr<RenderableMesh> mesh;
};

struct PlayerControllerComponent{
    using Key = InputManager::Key;
    // std::vector<std::pair<Key, glm::vec3>> movement_keybinds;

    Key forward_keybind;
    Key left_keybind;
    Key backward_keybind;
    Key right_keybind;
    Key jump_keybind;
    Key sprint_keybind;
    
    float velocity_move;
    float velocity_jump;
    float sprint_scale;
};

struct NPCControllerComponent{
    enum WaypointOrder { Sequential, Random };

    std::vector<glm::vec3> waypoints;
    int current_waypoint = 0;
    float velocity;
    float waitSeconds;
    WaypointOrder waypointOrder;
    float currentWait;
};

struct PointLightComponent{
    glm::vec3 position;
    glm::vec3 color;
};

struct CameraComponent{
    float fov;
    const float nearPlane;
    const float farPlane;
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 viewportMatrix;
    bool isPivot = false;
}; 

struct ThirdPersonCameraControllerComponent{
    // glm::vec3 lookAt = glm_aux::vec3_000;   // Point of interest
    glm::vec3 up = glm_aux::vec3_010;       // Local up-vector
    float distance = 15.0f;                 // Distance to point-of-interest
    float sensitivity = 0.005f;             // Mouse sensitivity

    // Position and view angles (computed when camera is updated)
    float yaw = 0.0f;                       // Horizontal angle (radians)
    float pitch = -glm::pi<float>() / 8;    // Vertical angle (radians)

    // Previous mouse position
    glm::ivec2 mouse_xy_prev{ -1, -1 };
}; 
