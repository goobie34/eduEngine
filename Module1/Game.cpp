#include <entt/entt.hpp>
#include "glmcommon.hpp"
#include "imgui.h"
#include "Game.hpp"
#include "CoreComponents.hpp"
#include "EventComponents.hpp"
#include "CollisionComponents.hpp"
#include "CollisionSystems.hpp"
#include "EventSystems.hpp"
#include "CoreSystems.hpp"
#include "ECS/GameComponents.hpp"
#include "ECS/GameSystems.hpp"
#include "Log.hpp"

bool Game::init()
{
    //Init renderers
    forwardRenderer = std::make_shared<eeng::ForwardRenderer>();
    forwardRenderer->init("shaders/phong_vert.glsl", "shaders/phong_frag.glsl");
    shapeRenderer = std::make_shared<ShapeRendering::ShapeRenderer>();
    shapeRenderer->init();

    //Load meshes
    m_environmentMesh = std::make_shared<eeng::RenderableMesh>();
    // grassMesh->load("assets/grass/grass_trees_merged.fbx", false);
    m_environmentMesh->load("assets/grass/prototype_ground.fbx", false);

    m_npcMesh = std::make_shared<eeng::RenderableMesh>();
    // horseMesh->load("assets/Animals/Horse.fbx", false);
    m_npcMesh->load("assets/Pete/Pete.fbx", false);
    m_npcMesh->load("assets/Pete/Happy Idle.fbx", true);

    m_characterMesh = std::make_shared<eeng::RenderableMesh>();//anim index
    m_characterMesh->load("assets/Chad/Chad.fbx");             //0   
    m_characterMesh->load("assets/Chad/Idle.fbx",    true);    //1
    m_characterMesh->load("assets/Chad/Walking.fbx", true);    //2
    m_characterMesh->load("assets/Chad/Running.fbx", true);    //3
    m_characterMesh->load("assets/Chad/Dancing.fbx", true);    //4
    m_characterMesh->removeTranslationKeys("mixamorig:Hips");     //remove root motion

    //Set up EnTT
    m_entity_registry = std::make_shared<entt::registry>();

    //Create entities
    m_playerEntity = m_entity_registry->create();
    m_cameraEntity = m_entity_registry->create();
    m_environmentEntity = m_entity_registry->create();
    m_npcEntity = m_entity_registry->create();
    m_npcProximityTriggerEntity = m_entity_registry->create();
    m_lightEntity = m_entity_registry->create();
    m_guiEntity = m_entity_registry->create();

    m_entity_registry->emplace<InfoComponent>(m_playerEntity, "player");
    m_entity_registry->emplace<InfoComponent>(m_cameraEntity, "camera");
    m_entity_registry->emplace<InfoComponent>(m_environmentEntity, "grass");
    m_entity_registry->emplace<InfoComponent>(m_npcEntity, "npc");
    m_entity_registry->emplace<InfoComponent>(m_npcProximityTriggerEntity, "npc proximity trigger");
    m_entity_registry->emplace<InfoComponent>(m_lightEntity, "light");
    m_entity_registry->emplace<InfoComponent>(m_guiEntity, "gui entity");

    //set up player input map
    using Key = InputManager::Key;
    InputMap playerInputMap{"Player Input Map"};
    playerInputMap.addKeybind("forward",  std::vector<Key>{Key::W, Key::Up});
    playerInputMap.addKeybind("left",     std::vector<Key>{Key::A, Key::Left});
    playerInputMap.addKeybind("backward", std::vector<Key>{Key::S, Key::Down});
    playerInputMap.addKeybind("right",    std::vector<Key>{Key::D, Key::Right});
    playerInputMap.addKeybind("sprint",   std::vector<Key>{Key::LeftShift});
    playerInputMap.addKeybind("interact", std::vector<Key>{Key::E});
    
    SourceComponent playerEventSource;
    playerEventSource.AddObserver(m_guiEntity);
    playerEventSource.AddObserver(m_npcEntity);
    playerEventSource.AddObserver(m_playerEntity);

    //Assign components to entities
    //PLAYER
    m_entity_registry->emplace<TransformComponent>(m_playerEntity,
        glm_aux::vec3_000, 0.0f, 0.0f, glm::vec3{0.03f, 0.03f, 0.03f});
    m_entity_registry->emplace<MeshComponent>(m_playerEntity, std::weak_ptr(m_characterMesh));
    m_entity_registry->emplace<SphereColliderComponent>(m_playerEntity);
    m_entity_registry->emplace<AABBColliderComponent>(m_playerEntity);
    m_entity_registry->emplace<LinearVelocityComponent>(m_playerEntity, LinearVelocityComponent{});
    m_entity_registry->emplace<PlayerControllerComponent>(
        m_playerEntity, playerInputMap);
    m_entity_registry->emplace<AnimationComponent>(m_playerEntity,
        0, 1, 1.0f, 1.0f, false, 0.0f, std::string("mixamorig:Spine"));
    m_entity_registry->emplace<PlayerAnimationControllerComponent>(m_playerEntity,
        1, 2, 3, 4.0f, 10.0f);
    m_entity_registry->emplace<PlayerInteractComponent>(m_playerEntity, playerInputMap);
    m_entity_registry->emplace<GUI_InventoryComponent>(m_playerEntity, "ItemName", 5, 5);
    m_entity_registry->emplace<SourceComponent>(m_playerEntity, playerEventSource);
    m_entity_registry->emplace<ObserverComponent>(m_playerEntity,
        [this] (Event event) {  
            //causes player to "drop" item when INTERACT event is called
            if (event.type == EventType::PLAYER_INTERACT) {
                if (auto inventory = m_entity_registry->try_get<GUI_InventoryComponent>(m_playerEntity)) {
                    inventory->drop();
                }
            }
        });

    //CAMERA
    m_entity_registry->emplace<TransformComponent>(m_cameraEntity,
        glm_aux::vec3_000, 0.0f, 0.0f, glm::vec3{1, 1, 1});
    m_entity_registry->emplace<CameraComponent>(m_cameraEntity,
        60.0f,  // fov
        1.0f,   // near
        500.0f, // far
        true,   // isMain
        true);  // isPivot
    m_entity_registry->emplace<ThirdPersonCameraControllerComponent>(m_cameraEntity,
        m_playerEntity); //lookAt entity

    //ENVIRONMENT
    m_entity_registry->emplace<TransformComponent>(m_environmentEntity,
        glm::vec3{0.0f, 0.0f, 0.0f},
        0.0f,   //pitch
        0.0f,   //yaw
        glm::vec3{100.0f, 100.0f, 100.0f});
    m_entity_registry->emplace<MeshComponent>(m_environmentEntity, std::weak_ptr(m_environmentMesh));

    //NPC
    m_entity_registry->emplace<TransformComponent>(m_npcEntity,
        glm::vec3{5.0f, 0.0f, -5.0f}, 0.0f, 0.0f, glm::vec3{0.03f, 0.03f, 0.03f});
    m_entity_registry->emplace<MeshComponent>(m_npcEntity, std::weak_ptr(m_npcMesh));
    m_entity_registry->emplace<SphereColliderComponent>(m_npcEntity, false, true);
    m_entity_registry->emplace<AABBColliderComponent>(m_npcEntity, false, true);
    m_entity_registry->emplace<AnimationComponent>(m_npcEntity,
        0, 1, 1.0f, 1.0f, false, 0.0f, std::string("mixamorig:Spine"));
    m_entity_registry->emplace<GUI_ProgressBarComponent>(m_npcEntity);
    m_entity_registry->emplace<ObserverComponent>(m_npcEntity,
        [this] (Event event) {
            //this lambda changes value of progressbar when INTERACT event is called, by the int value sent in the event payload
            if (event.type == EventType::PLAYER_INTERACT) {
                if (auto progressBar = m_entity_registry->try_get<GUI_ProgressBarComponent>(m_npcEntity)) {
                    progressBar->changeValue(event.data_int);
                }
            }
        });

    AABB npcTrigger;
    npcTrigger.min = glm::vec3{2.0f, 0.0f, -8.0f};
    npcTrigger.max = glm::vec3{8.0f, 1.0f, -2.0f};
    m_entity_registry->emplace<TransformComponent>(m_npcProximityTriggerEntity,
        glm::vec3{5.0f, 0.0f, -5.0f}, 0.0f, 0.0f, glm::vec3{1.0f, 1.0f, 1.0f});
        m_entity_registry->emplace<AABBColliderComponent>(m_npcProximityTriggerEntity,
            true,   //trigger
            true,   //static     
            false,  // setFromMesh
            npcTrigger
            ); 



    //POINT LIGHT
    m_entity_registry->emplace<PointLightComponent>(m_lightEntity,
        glm::vec3{0.0f, 5.0f, 0.0f},    //pos
        glm::vec3{1.0f, 1.0f, 1.0f});   //color
    
    //GUI
    m_entity_registry->emplace<GUI_QuestLogComponent>(m_guiEntity, std::vector<std::string>{"Talk to Pete!"});
    m_entity_registry->emplace<ObserverComponent>(m_guiEntity, 
        [this] (Event e) {
            if (auto questLog = m_entity_registry->try_get<GUI_QuestLogComponent>(m_guiEntity)) {
                    questLog->add(std::string(e.message + ": " + std::to_string(e.timeStamp)).c_str());
            }

            eeng::Log(e.message.c_str());
        }); //lambda called when this entity is notified, prints every event message to GUI log


    return true;
}

void Game::update(
    float time,
    float deltaTime,
    InputManagerPtr input)
{
    //Events
    ObserverSystem::Update(*m_entity_registry);

    //core
    MovementSystem::Update(deltaTime, *m_entity_registry);
    AnimationSystem::Update(deltaTime, *m_entity_registry);
    ColliderSystem::UpdateAABBs(*m_entity_registry);
    ColliderSystem::UpdateSpheres(*m_entity_registry);
    CollisionSystem::Update(*m_entity_registry);

    //game
    PlayerControllerSystem::Update(deltaTime, input, *m_entity_registry);
    PlayerInteractSystem::Update(time, input, *m_entity_registry);
    PlayerAnimationSystem::Update(*m_entity_registry);
    ThirdPersonCameraControllerSystem::Update(input, *m_entity_registry);
    NPCControllerSystem::Update(deltaTime, *m_entity_registry);
}

void Game::render(
    float time,
    int windowWidth,
    int windowHeight)
{
    //core
    CameraSystem::Update(windowWidth, windowHeight, *m_entity_registry);
    RenderSystem::Render(forwardRenderer, *m_entity_registry);
    if (m_renderGizmos) GizmoSystem::Render(shapeRenderer, *m_entity_registry);
    
    //game
    drawcallCount = forwardRenderer->endPass();
    GUI_ProgressBarSystem::Update(windowHeight, *m_entity_registry);
    GUI_InventorySystem::Update(*m_entity_registry);
    GUI_QuestLogSystem::Update(*m_entity_registry);
    renderUI();
}

void Game::renderUI()
{
    // Begin game info ImGui window
    ImGui::Begin("Entity Info");

    //Editor GUI
    ImGui::Text("Drawcall count %i", drawcallCount);
    auto view = m_entity_registry->view<InfoComponent>();
    for (auto entity : view) {
        auto info = view.get<InfoComponent>(entity);

        if (ImGui::CollapsingHeader(info.name.c_str())) {
            if (auto transform = m_entity_registry->try_get<TransformComponent>(entity)) {
                ImGui::Text("transform");
                ImGui::DragFloat3(std::string("Position##" + info.name).c_str(), &transform->position.x, 0.01f);
                ImGui::DragFloat3(std::string("Scale##" + info.name).c_str(), &transform->scale.x, 0.01f);
                ImGui::Separator();

            }
            if (auto cameraController = m_entity_registry->try_get<ThirdPersonCameraControllerComponent>(entity)) {
                ImGui::Text("camera controller");
                ImGui::SliderFloat(std::string("Camera Distance##" + info.name).c_str(), &cameraController->distance, 0.0f, 100.0f);
                ImGui::SliderFloat(std::string("Camera Offset##" + info.name).c_str(), &cameraController->offset, 0.0f, 100.0f);            
                ImGui::Separator();

            }
            if (auto playerController = m_entity_registry->try_get<PlayerControllerComponent>(entity)) {
                ImGui::Text("player controller");
                ImGui::DragFloat(std::string("Acceleration##" + info.name).c_str(), &playerController->acceleration_rate);
                ImGui::DragFloat(std::string("Friction##" + info.name).c_str(), &playerController->friction, 0.0f, 60);
                ImGui::Separator();

            }
            if (auto velocityComponent = m_entity_registry->try_get<LinearVelocityComponent>(entity)) {
                ImGui::Text("velocity component");
                float velocity = glm::length(velocityComponent->velocity);
                ImGui::Text("Current velocity: %.1f m/s", velocity); 
                ImGui::Separator();

            }
            if (auto sphereComponent = m_entity_registry->try_get<SphereColliderComponent>(entity)) {
                ImGui::Text("Sphere Collider Component");
                ImGui::Checkbox(std::string("IsStatic##" + info.name).c_str(), &sphereComponent->isStatic);
                ImGui::Checkbox(std::string("IsTrigger##" + info.name).c_str(), &sphereComponent->isTrigger);
                ImGui::Checkbox(std::string("SetFromMesh##" + info.name).c_str(), &sphereComponent->setFromMesh);
                ImGui::DragFloat3(std::string("Position##" + info.name).c_str(), &sphereComponent->pos.x, 0.01f);
                ImGui::DragFloat(std::string("Radius##" + info.name).c_str(), &sphereComponent->radius, 0.0f, 60);
                ImGui::Separator();

            }
            if (auto aabbComponent = m_entity_registry->try_get<AABBColliderComponent>(entity)) {
                ImGui::Text("AABB Collider Component");
                ImGui::Checkbox(std::string("IsStatic##" + info.name).c_str(), &aabbComponent->isStatic);
                ImGui::Checkbox(std::string("IsTrigger##" + info.name).c_str(), &aabbComponent->isTrigger);
                ImGui::Checkbox(std::string("SetFromMesh##" + info.name).c_str(), &aabbComponent->setFromMesh);
                ImGui::DragFloat3(std::string("Min##" + info.name).c_str(), &aabbComponent->aabb.min.x, 0.01f);
                ImGui::DragFloat3(std::string("Max##" + info.name).c_str(), &aabbComponent->aabb.min.x, 0.01f);
                ImGui::Separator();

            }
            if (auto observerComponent = m_entity_registry->try_get<ObserverComponent>(entity)) {
                ImGui::Text("Observer Component");
                ImGui::Separator();
            }
            if (auto sourceComponent = m_entity_registry->try_get<SourceComponent>(entity)) {
                ImGui::Text("Source Component");
                ImGui::Text("Observers:");
                ImGui::Indent();
                for(auto observer : sourceComponent->observers) {
                    if (auto observerInfo = m_entity_registry->try_get<InfoComponent>(entity)) {
                        ImGui::Text(std::string("- " + info.name).c_str());
                    }
                }
                ImGui::Unindent();
                ImGui::Separator();
            }
            if (auto animationController = m_entity_registry->try_get<PlayerAnimationControllerComponent>(entity)) {
                ImGui::Text("PlayerAnimationController Component");
                ImGui::SliderFloat(std::string("Velocity Threshold Walk##" + info.name).c_str(), &animationController->thresholdWalk, 0.0f, 100.0f);
                ImGui::SliderFloat(std::string("Velocity Threshold Run##" + info.name).c_str(), &animationController->thresholdRun, 0.0f, 100.0f);
                ImGui::Separator();            
            }
            if (auto animationComponent = m_entity_registry->try_get<AnimationComponent>(entity)) {
                ImGui::Text("Animation Component");

                ImGui::SliderFloat("Animation Blend", &animationComponent->blendFactor, 0.0f, 1.0f);
                ImGui::SliderInt("Animation Index A", &animationComponent->animIndexA, 0, 3);
                ImGui::SliderInt("Animation Index B", &animationComponent->animIndexB, 0, 3);
                ImGui::Checkbox("Use Layering", &animationComponent->useLayering);
                ImGui::Separator();
            }
            if (auto lightComponent = m_entity_registry->try_get<PointLightComponent>(entity)) {
                ImGui::Text("Point Light Component");

                if (ImGui::ColorEdit3("Light color", glm::value_ptr(lightComponent->color),
                ImGuiColorEditFlags_NoInputs)){}
                ImGui::DragFloat3("Light Pos X", &lightComponent->position.x, 0.1f);
                ImGui::Separator();
            }
        }
    }

    // if (ImGui::CollapsingHeader("Player Animation Settings")) {
    //     auto& animationController = *m_entity_registry->try_get<PlayerAnimationControllerComponent>(m_playerEntity);
    //     ImGui::SliderFloat("Velocity Threshold Walk", &animationController.thresholdWalk, 0.0f, 100.0f);
    //     ImGui::SliderFloat("Velocity Threshold Run", &animationController.thresholdRun, 0.0f, 100.0f);
        
    //     auto& animationComponent = *m_entity_registry->try_get<AnimationComponent>(m_playerEntity);
    //     ImGui::SliderFloat("Animation Blend", &animationComponent.blendFactor, 0.0f, 1.0f);
    //     ImGui::SliderInt("Animation Index A", &animationComponent.animIndexA, 0, 3);
    //     ImGui::SliderInt("Animation Index B", &animationComponent.animIndexB, 0, 3);
    //     ImGui::Checkbox("Use Layering", &animationComponent.useLayering);
    // }
    // //LIGHT
    // if (ImGui::CollapsingHeader("Light Settings")) {
    //     auto* lightPtr = m_entity_registry->try_get<PointLightComponent>(m_lightEntity);
    //     if (lightPtr) {
    //         auto& lightRef = *lightPtr;
    //         if (ImGui::ColorEdit3("Light color", glm::value_ptr(lightRef.color),
    //         ImGuiColorEditFlags_NoInputs))
    //         {}
    //         ImGui::DragFloat3("Light Pos X", &lightRef.position.x, 0.1f);
    //     }
    //     else {
    //         ImGui::Text("No Light Found.");
    //     }
    // }
    ImGui::Checkbox("Render Gizmos", &m_renderGizmos);

    ImGui::End(); // end info window
}

void Game::destroy()
{

}
