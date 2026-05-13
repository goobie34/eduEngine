#include "glmcommon.hpp"
#include "InputManager.hpp"
#include "CoreComponents.hpp"
#include "GameComponents.hpp"
#include <entt/entt.hpp>
#include <cassert>

#pragma once

class PlayerControllerSystem {
public:    
    static void Update(float dt, InputManagerPtr input, entt::registry& registry)
    {
        auto view = registry.view<PlayerControllerComponent, LinearVelocityComponent, TransformComponent>();
        
        for(auto entity : view)
        {
            //Get components
            auto& player_controller = view.get<PlayerControllerComponent>(entity);
            const auto& input_map = player_controller.inputMap;
            auto& velocity = view.get<LinearVelocityComponent>(entity).velocity; 
            auto& transform = view.get<TransformComponent>(entity); 

            //Get reference to ThirdPersonCameraControllerComponent
            auto cam_ctrl_ptr = GetCameraController(registry);
            if (cam_ctrl_ptr == nullptr) return;
            auto& cam_controller = *cam_ctrl_ptr;

            glm::vec3 fwd_dir = ComputeLocalFwd(cam_controller);
            glm::vec3 input_vec = GetInputVector(input_map, input, fwd_dir);

            if (glm::length(input_vec) > 0.0001f)
            {   
                //apply movement with acceleration
                bool is_sprinting = input_map.isPressed("sprint", input);
                glm::vec3 target_velocity =  GetTargetVelocity(player_controller, input_vec, is_sprinting);
                velocity = glm::mix(velocity, target_velocity, player_controller.acceleration_rate * dt);
            } else
            {
                //apply friction
                velocity *= glm::min(player_controller.friction * dt, 1.0f);
            }

            SetRotationFromVelocity(transform, velocity);
        }
    }
    static const ThirdPersonCameraControllerComponent* GetCameraController(entt::registry& registry) {
        auto cam_controller_view = registry.view<ThirdPersonCameraControllerComponent>();
        
        if (cam_controller_view.empty())
        {
            assert(false && "PlayerControllerSystem: There is no ThirdPersonCameraControllerComponent in entity registry.");
            return nullptr;
        }

        entt::entity cam_controller_entity = cam_controller_view.front();
        const ThirdPersonCameraControllerComponent* cam_controller = &registry.get<ThirdPersonCameraControllerComponent>(cam_controller_entity);
        return cam_controller;
    }  

    static glm::vec3 ComputeLocalFwd(const ThirdPersonCameraControllerComponent& cam_controller) {
        return glm::vec3(glm_aux::R(cam_controller.yaw, glm_aux::vec3_010) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
    } 

    static glm::vec3 GetInputVector(const InputMap& input_map, InputManagerPtr input, glm::vec3 fwd_dir) {
        bool forward   = input_map.isPressed("forward",  input);
        bool backward  = input_map.isPressed("backward", input);
        bool left      = input_map.isPressed("left",     input);
        bool right     = input_map.isPressed("right",    input);

        glm::vec3 right_dir = glm::cross(fwd_dir, glm_aux::vec3_010);

        //calculate move direction sum
        glm::vec3 input_vec = fwd_dir   * ((forward ? 1.0f : 0.0f) + (backward ? -1.0f : 0.0f)) +
                              right_dir * ((left ? -1.0f : 0.0f)   + (right ? 1.0f : 0.0f));

        return input_vec;
    }

    static glm::vec3 GetTargetVelocity(PlayerControllerComponent& player_controller, glm::vec3 input_vec, bool is_sprinting) {
        glm::vec3 move_dir = glm::normalize(input_vec);
        float speed_scale = is_sprinting ? player_controller.sprint_scale : 1.0f;
        glm::vec3 target_velocity =  move_dir * player_controller.move_speed * speed_scale;
        return target_velocity;
    }

    static void SetRotationFromVelocity(TransformComponent& transform, const glm::vec3& velocity) {
        if (glm::length(velocity) > 0.001f) {
            transform.yaw = std::atan2(velocity.x, velocity.z);    
        }
    }
};

class PlayerAnimationSystem {
public:
    static void Update(entt::registry& registry) {
        auto view = registry.view<LinearVelocityComponent, AnimationComponent, PlayerAnimationControllerComponent>();
        for(auto entity : view) {
            auto& velocity = view.get<LinearVelocityComponent>(entity).velocity;
            auto& animationComponent = view.get<AnimationComponent>(entity);
            auto& animationController = view.get<PlayerAnimationControllerComponent>(entity);
            float velocityMag = glm::length(velocity);
            if(velocityMag <= animationController.thresholdWalk) {
                animationComponent.animIndexA = animationController.animIndexIdle;
                animationComponent.animIndexB = animationController.animIndexWalk;
                animationComponent.blendFactor = glm::clamp(velocityMag / animationController.thresholdWalk, 
                                                 0.0f, 1.0f);
            } else if(velocityMag <= animationController.thresholdRun) {
                animationComponent.animIndexA = animationController.animIndexWalk;
                animationComponent.animIndexB = animationController.animIndexRun;
                animationComponent.blendFactor = 
                    glm::clamp((velocityMag - animationController.thresholdWalk)
                               / (animationController.thresholdRun - animationController.thresholdWalk),
                               0.0f, 1.0f);
            }
        }
    }
};

class ThirdPersonCameraControllerSystem {
public:
    static void Update(InputManagerPtr input, entt::registry& registry)
    {        
        auto view = registry.view<TransformComponent, CameraComponent, ThirdPersonCameraControllerComponent>();

        for(auto entity : view) {
            ThirdPersonCameraControllerComponent& controller = view.get<ThirdPersonCameraControllerComponent>(entity);
            TransformComponent& transform = view.get<TransformComponent>(entity);
            CameraComponent& camera = view.get<CameraComponent>(entity);

            auto entityView = registry.view<TransformComponent>();

            if (controller.lookAt == entt::null) continue;
            if (!entityView.contains(controller.lookAt)) continue; //throw error

            glm::vec3& lookAt_pos = entityView.get<TransformComponent>(controller.lookAt).position;

            // Fetch mouse and compute movement since last frame
            auto mouse = input->GetMouseState();
            glm::ivec2 mouse_xy{ mouse.x, mouse.y };
            glm::ivec2 mouse_xy_diff{ 0, 0 };
            if (mouse.rightButton && controller.mouse_xy_prev.x >= 0)
                mouse_xy_diff = controller.mouse_xy_prev - mouse_xy;
            controller.mouse_xy_prev = mouse_xy;

            // Update camera rotation from mouse movement
            controller.yaw += mouse_xy_diff.x * controller.sensitivity;
            controller.pitch += mouse_xy_diff.y * controller.sensitivity;
            controller.pitch = glm::clamp(controller.pitch, -glm::radians(89.0f), 0.0f);

            // Update camera position
            const glm::vec4 rotatedPos = glm_aux::R(controller.yaw, controller.pitch) * glm::vec4(0.0f, 0.0f, controller.distance, 1.0f);
            transform.position = lookAt_pos + glm::vec3(rotatedPos);
            camera.lookAt_pos = lookAt_pos; //and tell it where to look
        }
    }
};

class NPCControllerSystem{
public:
    static void Update(float dt, entt::registry& registry) {
        auto view = registry.view<NPCControllerComponent, TransformComponent>();
        for(auto entity : view) {
            auto& transform = view.get<TransformComponent>(entity);
            auto& npc_controller = view.get<NPCControllerComponent>(entity);
        
            int& waypoint_index = npc_controller.current_waypoint;
            int nr_waypoints = npc_controller.waypoints.size();
            
            const glm::vec3 waypoint = npc_controller.waypoints[waypoint_index];
            const glm::vec3 difference = waypoint - transform.position;
            if (glm::length(difference) > 0.4f) {
                const glm::vec3 direction = glm::normalize(difference);
                transform.position += direction * npc_controller.velocity * dt;
                
                float targetYaw = std::atan2(direction.x, direction.z); //get rotation around y axis (yaw) from velocity
                
                transform.yaw = targetYaw;
            } else {
                float& curr_wait = npc_controller.currentWait;
                const float max_wait = npc_controller.waitSeconds;

                if (curr_wait < max_wait) {
                    curr_wait += dt;
                } else {
                    curr_wait = 0;
                    waypoint_index = (waypoint_index + 1) % nr_waypoints;
                }
            }
        }
    }
};

static class PlayerInteractSystem {
public:
    static void Update(float time, InputManagerPtr input, entt::registry& registry) {
        auto view = registry.view<PlayerInteractComponent>();
        for(auto entity : view) {
            auto& interactComponent = view.get<PlayerInteractComponent>(entity);
            if (interactComponent.inputMap.isPressed("interact", input)) {
                if (auto source = registry.try_get<SourceComponent>(entity)) {
                    source->AddEvent(Event{EventType::PLAYER_INTERACT, time, "Player has interacted!", 0, 0.0f, entity});
                }
            }
        }
    }
};