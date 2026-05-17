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

            if(!animationController.active) continue;
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

            glm::vec3 lookAt_pos = entityView.get<TransformComponent>(controller.lookAt).position;
            lookAt_pos.y += controller.offset;

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
    static void Update(float time, float dt, InputManagerPtr input, entt::registry& registry) {
        auto view = registry.view<PlayerInteractComponent>();
        for(auto entity : view) {
            auto& interactComponent = view.get<PlayerInteractComponent>(entity);
            
            interactComponent.timer -= dt;
            if(interactComponent.timer <= 0) continue;
            
            if (interactComponent.inputMap.isPressed("interact", input)) {
                if (!interactComponent.wasInteracting)
                {
                    if (auto source = registry.try_get<SourceComponent>(entity))
                    {
                        source->AddEvent(Event{EventType::PLAYER_INTERACT, time, "Player has interacted!", 1, 0.0f, entity});
                    }     
                }
                interactComponent.wasInteracting = true;
            } else {
                interactComponent.wasInteracting = false;
            }
        }
    }
};

static class GUI_ProgressBarSystem {
public:
    static void Update(float windowHeight, entt::registry& registry) {
        entt::entity mainCam_id = RenderSystem::getMainCam(registry);
        const auto mainCam = registry.get<CameraComponent>(mainCam_id);
        const auto VP_P_V = mainCam.viewportMatrix * mainCam.projectionMatrix * mainCam.viewMatrix;
        
        auto view = registry.view<TransformComponent, GUI_ProgressBarComponent>();

        for(auto entity : view) {
            const auto transform = registry.get<TransformComponent>(entity);
            auto progressBar = registry.get<GUI_ProgressBarComponent>(entity);

            auto world_pos = transform.position + progressBar.offset;
            glm::ivec2 window_coords;

            if (glm_aux::window_coords_from_world_pos(world_pos, VP_P_V, window_coords))
            {
                // Draw an ImGui label at the projected window coordinates of the horse
                ImGui::SetNextWindowPos(
                    ImVec2{ float(window_coords.x), float(windowHeight - window_coords.y) },
                    ImGuiCond_Always,
                    ImVec2{ 0.0f, 0.0f });
                ImGui::PushStyleColor(ImGuiCol_WindowBg, 0x80000000);
                ImGui::PushStyleColor(ImGuiCol_Text, 0xffffffff);

                ImGuiWindowFlags flags =
                    ImGuiWindowFlags_NoDecoration |
                    ImGuiWindowFlags_NoInputs |
                    // ImGuiWindowFlags_NoBackground |
                    ImGuiWindowFlags_AlwaysAutoResize;

                if (ImGui::Begin("window_name", nullptr, flags))
                {
                    ImGui::Text("Items gathered");
                    ImGui::ProgressBar(progressBar.current / progressBar.max);
                    
                    // ImGui::Text("Window pos (%i, %i)", window_coords.x, window_coords.y);
                    // ImGui::Text("World pos (%1.1f, %1.1f, %1.1f)", world_pos.x, world_pos.y, world_pos.z);
                    ImGui::End();
                }
                ImGui::PopStyleColor(2);
            }
        }
    }
};

static class GUI_InventorySystem {
public:
    static void Update(entt::registry& registry) {
        auto view = registry.view<GUI_InventoryComponent>();
        for(auto entity : view) {
            auto inventory = registry.get<GUI_InventoryComponent>(entity);

            // ImGui::SetNextWindowPos(
            //     ImVec2{ 0.5f, 0.5f },
            //     ImGuiCond_Always,
            //     ImVec2{ 0.0f, 0.0f });
            // ImGui::PushStyleColor(ImGuiCol_WindowBg, 0x80000000);
            // ImGui::PushStyleColor(ImGuiCol_Text, 0xffffffff);

            ImGui::Begin(std::string("Inventory").c_str());
            ImGui::TextColored(ImVec4(1, 0, 0, 1), std::string(inventory.itemName + ": " + std::to_string(inventory.current)).c_str());
            ImGui::End();
        }
    }
};
static class GUI_QuestLogSystem {
public:
    static void Update(entt::registry& registry) {
        auto view = registry.view<GUI_QuestLogComponent>();
        for(auto entity : view) {
            auto questLog = registry.get<GUI_QuestLogComponent>(entity);
            ImGui::Begin("Quest Log");
            for(int i = 0; i < questLog.log.size(); i++) {
                ImVec4 color;
                if (i == (questLog.log.size() - 1)) {
                    color = {0, 1, 0, 1};
                } else {
                    color = {1, 1, 1, 1};
                }
                
                ImGui::TextColored(color,
                    std::string(std::to_string(i + 1) + ". " + questLog.log[i]).c_str());

            }

            ImGui::End();
        }
    }
};

static class StarSystem {
public:
    static void Update(float time, float dt, entt::registry& registry) {
        auto view = registry.view<StarComponent, TransformComponent>();
        for(auto entity : view) {
            auto& star = registry.get<StarComponent>(entity);
            auto& transform = registry.get<TransformComponent>(entity);
            
            if(star.collected) continue;

            transform.position.y = star.baseHeight + std::sin(time * star.vertical_movement_speed) * star.vertical_movement;
            transform.yaw += star.rotation_speed * dt;
        }
    }
};

static class GUI_InteractPromptSystem {
public:
    static void Update(float windowHeight, entt::registry& registry, float dt) {
        auto view = registry.view<TransformComponent, GUI_InteractPrompt>();

        for(auto entity : view) {
            const auto transform = registry.get<TransformComponent>(entity);
            auto& interactPrompt = registry.get<GUI_InteractPrompt>(entity);

            interactPrompt.timer -= dt;
            if(interactPrompt.timer <= 0) continue;
            ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoDecoration |
                // ImGuiWindowFlags_NoInputs |
                // ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_AlwaysAutoResize;

            if (ImGui::Begin(("Tip:##" + std::to_string((uint32_t)entity)).c_str(), nullptr, flags))
            {
                ImGui::Text(interactPrompt.prompt.c_str());
                ImGui::End();
            }
        }
    }
};

static class QuestSystem {
public:
    static void Update(entt::registry& registry) {
        auto view = registry.view<QuestComponent, PeteNPCComponent, AnimationComponent, SourceComponent>();
        for(auto entity : view) {
            auto& quest = view.get<QuestComponent>(entity);
            auto& pete = view.get<PeteNPCComponent>(entity);
            auto& animation = view.get<AnimationComponent>(entity);
            auto& source = view.get<SourceComponent>(entity);
            //queststage = 0;
            if(pete.hasInteractedOnce) {
                quest.stage = 1;
            }
            
            if(pete.hasAllStars) {
                quest.stage = 2;
            } 

            if(pete.shouldDance) {
                quest.stage = 3;
                animation.blendFactor = 1.0f;
                source.AddEvent(Event(EventType::QUEST_OVER, 0.0f, "QUEST COMPLETED", 0, 0.0f, entity));
            }
            AddToQuestLog(registry, quest);
        }
    }
    static void AddToQuestLog(entt::registry& registry, QuestComponent& questComponent) {
        auto view = registry.view<GUI_QuestLogComponent>();
        for(auto entity : view) {
            auto& questLog = view.get<GUI_QuestLogComponent>(entity);
            int currentLog = questLog.log.size() - 1;
            if (currentLog < questComponent.stage) {
                questLog.add(questComponent.quests[questComponent.stage]);
            }
        }
    }
};

