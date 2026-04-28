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
        auto view = registry.view<PlayerControllerComponent,
            LinearVelocityComponent, 
            TransformComponent>();
        
        for(auto entity : view)
        {
            auto& player_controller = view.get<PlayerControllerComponent>(entity);
            auto& velocity = view.get<LinearVelocityComponent>(entity).velocity; 
            auto& transform = view.get<TransformComponent>(entity); 

            bool forward    = input->IsKeyPressed(player_controller.forward_keybind);
            bool backward   = input->IsKeyPressed(player_controller.backward_keybind);
            bool left       = input->IsKeyPressed(player_controller.left_keybind);
            bool right      = input->IsKeyPressed(player_controller.right_keybind);
            bool sprint     = input->IsKeyPressed(player_controller.sprint_keybind);
            float scale = sprint ? player_controller.sprint_scale : 1.0f;

            //get third person camera controller, needed to set fwd_dir for player
            auto cam_controller_view = registry.view<ThirdPersonCameraControllerComponent>();
            if (cam_controller_view.empty()) {
                assert(false && "PlayerControllerComponent: There is no ThirdPersonCameraControllerComponent in entity registry.");
                return;
            }

            entt::entity cam_controller_entity = cam_controller_view.front();
            auto& cam_controller = registry.get<ThirdPersonCameraControllerComponent>(cam_controller_entity); 

            // Compute vectors in the local space of the player
            glm::vec3 fwd_dir = glm::vec3(glm_aux::R(cam_controller.yaw, glm_aux::vec3_010) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
            glm::vec3 right_dir = glm::cross(fwd_dir, glm_aux::vec3_010);

            glm::vec3 dir_sum = fwd_dir   * ((forward ? 1.0f : 0.0f) + (backward ? -1.0f : 0.0f)) +
                                right_dir * ((left ? -1.0f : 0.0f)   + (right ? 1.0f : 0.0f));
            
            if (glm::length(dir_sum) > 0.0001f)
            {
                float acceleration_rate = 4.0f;
                glm::vec3 move_dir = glm::normalize(dir_sum);
                glm::vec3 target_velocity =  move_dir * player_controller.move_speed * scale;
                velocity = glm::mix(velocity, target_velocity, acceleration_rate * dt);
            } else
            {
                velocity *= 0.9f;
            }

            //makes player face in movement direction
            if (glm::length(velocity) > 0.001f) {
                //set rotation around y axis (yaw) from velocity vector
                float rotationSpeed = 8.0f;
                float currentYaw = transform.yaw;
                float targetYaw = std::atan2(velocity.x, velocity.z);
                
                // float currentYawDeg = glm::degrees(currentYaw);
                // float targetYawDeg = glm::degrees(targetYaw);
                // if (glm::abs(targetYawDeg - currentYawDeg) > 180) {
                //     targetYawDeg = 360 - targetYaw;
                // }
                // targetYaw = glm::radians(targetYawDeg);

                // transform.yaw = glm::mix(currentYaw, targetYaw, rotationSpeed * dt);    
                transform.yaw = targetYaw;    
            }
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
                animationComponent.blendFactor = glm::clamp(velocityMag / animationController.thresholdWalk, 0.0f, 1.0f);

            } else if(velocityMag <= animationController.thresholdRun) {
                animationComponent.animIndexA = animationController.animIndexWalk;
                animationComponent.animIndexB = animationController.animIndexRun;
                animationComponent.blendFactor = 
                    glm::clamp((velocityMag - animationController.thresholdWalk)
                               / (animationController.thresholdRun - animationController.thresholdWalk)
                               , 0.0f, 1.0f);
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