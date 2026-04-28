#include "glmcommon.hpp"
#include "InputManager.hpp"
#include "CoreComponents.hpp"
#include <entt/entt.hpp>

#pragma once

using namespace eeng;

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

//currently, this cam controller needs to be on the player entity
//TODO: change it to sit on the camera entity, holding an entt:entity reference to the player, instead of the camera
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
    PlayerControllerComponent(Key fwd, Key left, Key back, Key right,Key sprint,
                              float move_speed = 5.0f, float sprint_scale = 4.0f) : forward_keybind(fwd),
                              left_keybind(left), backward_keybind(back), right_keybind(right),
                              sprint_keybind(sprint), move_speed(move_speed), sprint_scale(sprint_scale) {}

    Key forward_keybind;
    Key left_keybind;
    Key backward_keybind;
    Key right_keybind;
    Key sprint_keybind;
    
    float move_speed;
    float sprint_scale;
};

struct PlayerAnimationControllerComponent{
    int animIndexIdle, animIndexWalk, animIndexRun;
    float thresholdWalk, thresholdRun;
};