#include "glmcommon.hpp"
#include "InputManager.hpp"
#include "InputMap.hpp"
#include "CoreComponents.hpp"
#include <entt/entt.hpp>

#pragma once

using namespace eeng;

enum EventType : uint8_t {
    //CORE EVENTS
    LOOSE_COLLISION,
    TIGHT_COLLISION,
    TRIGGER,
    //GAME SPECIFIC
    PLAYER_INTERACT,
    QUEST_UPDATE,
    ITEM_PICKUP,
    ITEM_DROPOFF,
    QUEST_OVER
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
        entt::entity lookAt, float offset = 0.0f, float distance = 15.0f, float sensitivity = 0.005f)
        : offset(offset), lookAt(lookAt), distance(distance), sensitivity(sensitivity) {}

    //set in constructor
    entt::entity lookAt;
    float distance;
    float sensitivity;
    float offset;

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
    bool active = true;
};

struct PlayerInteractComponent {
    InputMap inputMap;
    bool wasInteracting = false;
    float timer = 0; //when this is more than 0, interacting is possible
    float interactWindow = 1.0f;
    void StartTimer() {timer = interactWindow;}
};

struct GUI_ProgressBarComponent {
    float max = 6.0f;
    float current = 0.0f;

    glm::vec3 offset = glm_aux::vec3_010 * 6.0f; //does nothing atm
    void changeValue(float change) {
        current += change;
        if (current > max) current = max;
        if (current < 0) current = 0;
    }
    // void changeValue(Event event) {
    //     current += event.data_int;
    //     if (current > max) current = max;
    //     if (current < 0) current = 0;
    // }
};
struct GUI_InventoryComponent {
    std::string itemName = "";
    int capacity = 99;
    int current = 0;

    void pickUp(int val = 1) {
        current += val;
        if (current > capacity) current = capacity;
    }
    void drop(int val = 1) {
        current -= val;
        if (current < 0) current = 0;
    }
};
struct GUI_QuestLogComponent {
    std::vector<std::string> log;
    int capacity = 255;
    void add(std::string logUpdate) {
        if (log.size() < capacity) {
            log.push_back(logUpdate);
        }
    }
};

struct StarComponent {
    float baseHeight;
    float rotation_speed = 2.0f;
    float vertical_movement = 0.5;
    float vertical_movement_speed = 2.0f;
    bool collected = false;
};

struct GUI_InteractPrompt {
    std::string prompt = "";
    float timer = 0; //when this is more than 0, prompt is visible
    float duration = 1.0f;
    void Start() {timer = duration;}
};

struct QuestComponent {
    int stage = 0;
    std::vector<std::string> quests = {"Talk to Pete", "Collect 6 stars and give them to Pete!", "Try talking to him one more time...", "~ Party ~"};
};

struct PeteNPCComponent {
    bool hasInteractedOnce = false;
    bool hasAllStars = false;
    bool shouldDance = false;
};