#include "glmcommon.hpp"
#include "InputManager.hpp"
#include "InputMap.hpp"
#include "CoreComponents.hpp"
#include <entt/entt.hpp>

#pragma once

using namespace eeng;

enum EventType : uint8_t {
    PLAYER_INTERACT
};

struct NPCControllerComponent{
    enum WaypointOrder { Sequential, Random };
    NPCControllerComponent(std::vector<glm::vec3> waypoints, float velocity = 5.0f,
                            float waitSeconds = 1.0f, WaypointOrder waypointOrder = WaypointOrder::Sequential)
                            : waypoints(waypoints), velocity(velocity),
                            waitSeconds(waitSeconds), waypointOrder(waypointOrder) {}

    //set in constructor
    std::vector<glm::vec3> waypoints;
    float velocity;
    float waitSeconds;
    WaypointOrder waypointOrder;

    //npc state
    int current_waypoint = 0;
    float currentWait = 0;
};

struct ThirdPersonCameraControllerComponent{
    ThirdPersonCameraControllerComponent(
        entt::entity lookAt, float distance = 15.0f, float sensitivity = 0.005f)
        : lookAt(lookAt), distance(distance), sensitivity(sensitivity) {}

    //set in constructor
    entt::entity lookAt;
    float distance;
    float sensitivity;

    //defaults
    float yaw = 0.0f;
    float pitch = -glm::pi<float>() / 8;
    glm::ivec2 mouse_xy_prev{ -1, -1 };
}; 

struct PlayerControllerComponent{
    using Key = InputManager::Key;
    PlayerControllerComponent(InputMap inputMap, float move_speed = 5.0f, float sprint_scale = 4.0f,
                              float friction = 54.0f, float acceleration_rate = 4.0f) : inputMap(inputMap), 
                              move_speed(move_speed),sprint_scale(sprint_scale), friction(friction),
                              acceleration_rate(acceleration_rate) {}
    InputMap inputMap;

    float move_speed;
    float sprint_scale;
    float friction;
    float acceleration_rate;
};

struct PlayerAnimationControllerComponent{
    int animIndexIdle, animIndexWalk, animIndexRun;
    float thresholdWalk, thresholdRun;
};

struct PlayerInteractComponent {
    InputMap inputMap;
};