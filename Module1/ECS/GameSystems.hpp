#include "glmcommon.hpp"
#include "InputManager.hpp"
#include "CoreComponents.hpp"
#include "GameComponents.hpp"
#include <entt/entt.hpp>

#pragma once

class PlayerControllerSystem {
public:    
    static void Update(InputManagerPtr input, entt::registry& registry)
    {
        auto view = registry.view<PlayerControllerComponent,
            LinearVelocityComponent, 
            ThirdPersonCameraControllerComponent,
            TransformComponent>();
        
        for(auto entity : view)
        {
            auto& player_controller = view.get<PlayerControllerComponent>(entity);
            auto& cam_controller = view.get<ThirdPersonCameraControllerComponent>(entity); 
            auto& velocity = view.get<LinearVelocityComponent>(entity).velocity; 
            auto& transform = view.get<TransformComponent>(entity); 

            bool forward    = input->IsKeyPressed(player_controller.forward_keybind);
            bool backward   = input->IsKeyPressed(player_controller.backward_keybind);
            bool left       = input->IsKeyPressed(player_controller.left_keybind);
            bool right      = input->IsKeyPressed(player_controller.right_keybind);
            bool sprint     = input->IsKeyPressed(player_controller.sprint_keybind);
            float scale = sprint ? player_controller.sprint_scale : 1.0f;

            // Compute vectors in the local space of the player
            glm::vec3 fwd_dir = glm::vec3(glm_aux::R(cam_controller.yaw, glm_aux::vec3_010) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
            glm::vec3 right_dir = glm::cross(fwd_dir, glm_aux::vec3_010);

            velocity =
                fwd_dir   * player_controller.velocity * scale * ((forward ? 1.0f : 0.0f) + (backward ? -1.0f : 0.0f)) +
                right_dir * player_controller.velocity * scale * ((left ? -1.0f : 0.0f)   + (right ? 1.0f : 0.0f));

            //makes player face in movement direction
            if (glm::length(velocity) > 0.001f) {
                //set rotation around y axis (yaw) from velocity vector
                float targetYaw = std::atan2(velocity.x, velocity.z);
                transform.rotation = glm_aux::R(targetYaw, glm_aux::vec3_010);
            }
        }
    }
};

class ThirdPersonCameraControllerSystem {
public:
    static void Update(InputManagerPtr input, entt::registry& registry)
    {        
        auto view = registry.view<TransformComponent, ThirdPersonCameraControllerComponent>();

        for(auto entity : view) {
            ThirdPersonCameraControllerComponent& controller = view.get<ThirdPersonCameraControllerComponent>(entity);
            TransformComponent& transform = view.get<TransformComponent>(entity);
            auto camView = registry.view<CameraComponent, TransformComponent>();

            if (controller.camera == entt::null) continue;
            if (!camView.contains(controller.camera)) continue; //throw error

            glm::vec3& cam_pos = camView.get<TransformComponent>(controller.camera).position;
            CameraComponent& camera = camView.get<CameraComponent>(controller.camera);

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
            cam_pos = transform.position + glm::vec3(rotatedPos);
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
                
                transform.rotation = glm_aux::R(targetYaw, glm_aux::vec3_010);
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