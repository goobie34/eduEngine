#include "glmcommon.hpp"
#include "RenderableMesh.hpp"
#include "InputManager.hpp"
#include "CoreComponents.hpp"
#include <entt/entt.hpp>
#include <ForwardRenderer.hpp>

#pragma once

using namespace eeng;

class MovementSystem {
public:    
    static void Update(float dt, entt::registry& registry)
    {
        auto view = registry.view<TransformComponent, LinearVelocityComponent>();
        
        for(auto entity : view) {
            auto& transform = view.get<TransformComponent>(entity);
            auto& velocity = view.get<LinearVelocityComponent>(entity).velocity;
            transform.position += velocity * dt;
        }
    }
};

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
            auto& controller = view.get<PlayerControllerComponent>(entity);
            auto& velocity = view.get<LinearVelocityComponent>(entity).velocity; 
            auto& cam_controller = view.get<ThirdPersonCameraControllerComponent>(entity); 
            auto& transform = view.get<TransformComponent>(entity); 

            bool forward    = input->IsKeyPressed(controller.forward_keybind);
            bool backward   = input->IsKeyPressed(controller.backward_keybind);
            bool left       = input->IsKeyPressed(controller.left_keybind);
            bool right      = input->IsKeyPressed(controller.right_keybind);
            bool jump       = input->IsKeyPressed(controller.jump_keybind);
            bool sprint     = input->IsKeyPressed(controller.sprint_keybind);

            float scale = sprint ? controller.sprint_scale : 1.0f;

            // Compute vectors in the local space of the player
            glm::vec3 fwd_dir = glm::vec3(glm_aux::R(cam_controller.yaw, glm_aux::vec3_010) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
            glm::vec3 right_dir = glm::cross(fwd_dir, glm_aux::vec3_010);

            velocity =
                fwd_dir   * controller.velocity_move * scale * ((forward ? 1.0f : 0.0f) + (backward ? -1.0f : 0.0f)) +
                right_dir * controller.velocity_move * scale * ((left ? -1.0f : 0.0f) + (right ? 1.0f : 0.0f));
                // + glm_aux::vec3_010 * controller.velocity_jump * (jump ? 1.0f : 0.0f);

            //make player face in camera direction
            if (glm::length(velocity) > 0.001f) {
                float targetYaw = std::atan2(velocity.x, velocity.z); //get rotation around y axis (yaw) from velocity
                transform.rotation = glm_aux::R(targetYaw, glm_aux::vec3_010);
                // transform.rotation = glm_aux::R(cam_controller.yaw + glm::pi<float>(), glm_aux::vec3_010);
            }

            // for(auto &[key, direction] : controller.movement_keybinds)
            // {w
            //     if (inputManagerPtr->IsKeyPressed(key))
            //         velocity += direction * controller.velocity_move;
            // }
        }
    }
};

class RenderSystem {
public:
    static void Render(eeng::ForwardRendererPtr forwardRenderer, entt::registry& registry)
    {        
        //let us assume we have 1 camera and 1 light source
        auto camView = registry.view<CameraComponent , TransformComponent>();
        const glm::vec3& cam_pos = camView.get<TransformComponent>(camView.front()).position;
        const CameraComponent& camera = camView.get<CameraComponent>(camView.front());

        auto lightView = registry.view<PointLightComponent>();
        const glm::vec3& light_pos = lightView.get<PointLightComponent>(lightView.front()).position;
        const glm::vec3& light_color = lightView.get<PointLightComponent>(lightView.front()).color;
        
        forwardRenderer->beginPass(camera.projectionMatrix, camera.viewMatrix, light_pos, light_color, cam_pos);

        auto meshView = registry.view<TransformComponent, MeshComponent>();
        for(auto entity : meshView)
        {
            const std::weak_ptr mesh = meshView.get<MeshComponent>(entity).mesh;
            const TransformComponent& transform = meshView.get<TransformComponent>(entity);

            if(auto mesh_ptr = mesh.lock()) //gets shared_ptr from weak_ptr, which is then released when scope ends
            {
                forwardRenderer->renderMesh(mesh_ptr, glm_aux::T(transform.position) * glm::mat4(transform.rotation) * glm_aux::S(transform.scale));
            }
        }
        
    }
};

class CameraSystem {
public:
    static void Update(int windowWidth, int windowHeight, entt::registry& registry)
    {        
        auto camView = registry.view<CameraComponent, TransformComponent>();
        const glm::vec3& cam_pos = camView.get<TransformComponent>(camView.front()).position;
        const CameraComponent& camera = camView.get<CameraComponent>(camView.front());

        auto view = registry.view<CameraComponent, TransformComponent>();
        for(auto entity : view)
        {
            CameraComponent& camera = view.get<CameraComponent>(entity);
            const TransformComponent& transform = view.get<TransformComponent>(entity);
            
            const float aspectRatio = float(windowWidth) / windowHeight;
            camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), aspectRatio, camera.nearPlane, camera.farPlane);
            camera.viewportMatrix = glm_aux::create_viewport_matrix(0.0f, 0.0f, windowWidth, windowHeight, 0.0f, 1.0f);
            
            //TODO: Fix third person camera
            
            if (!camera.isPivot)
                camera.viewMatrix = glm::mat4(glm::transpose(transform.rotation)) * glm_aux::T(1.0f * (transform.position));
        }
    }
};

class ThirdPersonCameraControllerSystem {
public:
    static void Update(InputManagerPtr input, entt::registry& registry)
    {        
        auto camView = registry.view<CameraComponent, TransformComponent>();
        glm::vec3& cam_pos = camView.get<TransformComponent>(camView.front()).position;
        CameraComponent& camera = camView.get<CameraComponent>(camView.front());

        auto view = registry.view<TransformComponent, ThirdPersonCameraControllerComponent>();
        TransformComponent& transform = view.get<TransformComponent>(view.front());
        ThirdPersonCameraControllerComponent& controller = view.get<ThirdPersonCameraControllerComponent>(view.front());

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

        camera.viewMatrix = glm::lookAt(cam_pos, transform.position, controller.up);

        // // Update camera position
        // const glm::vec4 rotatedPos = glm_aux::R(controller.yaw, controller.pitch) * glm::vec4(0.0f, 0.0f, controller.distance, 1.0f);
        // transform.position = controller.lookAt + glm::vec3(rotatedPos);
        // transform.rotation = glm_aux::T(-transform.position) * glm::mat4(glm::transpose(transform.rotation));
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
            if (glm::length(difference) > 0.1f) {
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