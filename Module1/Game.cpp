#include <entt/entt.hpp>
#include "glmcommon.hpp"
#include "imgui.h"
#include "Game.hpp"
#include "CoreComponents.hpp"
#include "CoreSystems.hpp"
#include "ECS/GameComponents.hpp"
#include "ECS/GameSystems.hpp"

bool Game::init()
{
    //Init renderers
    forwardRenderer = std::make_shared<eeng::ForwardRenderer>();
    forwardRenderer->init("shaders/phong_vert.glsl", "shaders/phong_frag.glsl");
    shapeRenderer = std::make_shared<ShapeRendering::ShapeRenderer>();
    shapeRenderer->init();

    //Load meshes
    grassMesh = std::make_shared<eeng::RenderableMesh>();
    grassMesh->load("assets/grass/grass_trees_merged.fbx", false);

    horseMesh = std::make_shared<eeng::RenderableMesh>();
    // horseMesh->load("assets/Animals/Horse.fbx", false);
    horseMesh->load("assets/Chad/Chad.fbx");
    horseMesh->load("assets/Chad/Idle.fbx", true);
    horseMesh->load("assets/Chad/Walking.fbx", true);
    horseMesh->load("assets/Chad/Running.fbx", true);
    horseMesh->load("assets/Chad/Dancing.fbx", true);

    characterMesh = std::make_shared<eeng::RenderableMesh>();
    // characterMesh->load("assets/Amy/Ch46_nonPBR.fbx");      //anim index 0
    // characterMesh->load("assets/Amy/idle.fbx", true);       //1    
    // characterMesh->load("assets/Amy/walking.fbx", true);    //2
    // characterMesh->load("assets/Amy/running.fbx", true);    //3
    // // characterMesh->load("assets/Amy/waving.fbx", true); 
    // characterMesh->removeTranslationKeys("mixamorig:Hips"); 
    
    // characterMesh->load("assets/ExoRed/exo_red.fbx");
    // characterMesh->load("assets/ExoRed/idle (2).fbx", true);
    // characterMesh->load("assets/ExoRed/walking.fbx", true);
    // characterMesh->load("assets/ExoRed/running.fbx", true);
    // // Remove root motion
    // characterMesh->removeTranslationKeys("mixamorig:Hips");

    characterMesh->load("assets/Chad/Chad.fbx");
    characterMesh->load("assets/Chad/Idle.fbx", true);
    characterMesh->load("assets/Chad/Walking.fbx", true);
    characterMesh->load("assets/Chad/Running.fbx", true);
    characterMesh->load("assets/Chad/Dancing.fbx", true);
    // Remove root motion
    characterMesh->removeTranslationKeys("mixamorig:Hips");

    //Set up EnTT
    m_entity_registry = std::make_shared<entt::registry>();

    //Create entities
    m_playerEntity = m_entity_registry->create();
    m_cameraEntity = m_entity_registry->create();
    m_grassEntity = m_entity_registry->create();
    m_npcEntity = m_entity_registry->create();
    m_npcEntity = m_entity_registry->create();
    m_lightEntity = m_entity_registry->create();

    //Assign components to entities
    //PLAYER
    m_entity_registry->emplace<TransformComponent>(m_playerEntity,
        glm_aux::vec3_000,
        0.0f,
        0.0f,
        glm::vec3{0.03f, 0.03f, 0.03f});
    m_entity_registry->emplace<MeshComponent>(m_playerEntity, std::weak_ptr(characterMesh));
    m_entity_registry->emplace<LinearVelocityComponent>(m_playerEntity, LinearVelocityComponent{});
    using Key = InputManager::Key;
    m_entity_registry->emplace<PlayerControllerComponent>(
        m_playerEntity, Key::W, Key::A, Key::S, Key::D, Key::LeftShift);
    m_entity_registry->emplace<AnimationComponent>(m_playerEntity,
        0, 1, 1.0f, 1.0f, false, 0.0f, std::string("mixamorig:Spine"));
    m_entity_registry->emplace<PlayerAnimationControllerComponent>(m_playerEntity,
        1, 2, 3, 4.0f, 10.0f);

    //CAMERA
    m_entity_registry->emplace<TransformComponent>(m_cameraEntity,
        glm_aux::vec3_000,
        0.0f,
        0.0f,
        glm::vec3{1, 1, 1});
    m_entity_registry->emplace<CameraComponent>(m_cameraEntity,
        60.0f,  // fov
        1.0f,   // near
        500.0f, // far
        true,   // isMain
        true);  // isPivot
    m_entity_registry->emplace<ThirdPersonCameraControllerComponent>(m_cameraEntity,
        m_playerEntity); //lookAt entity

    //ENVIRONMENT
    m_entity_registry->emplace<TransformComponent>(m_grassEntity,
        glm::vec3{0.0f, 0.0f, 0.0f},
        0.0f,   //pitch
        0.0f,   //yaw
        glm::vec3{100.0f, 100.0f, 100.0f});
    m_entity_registry->emplace<MeshComponent>(m_grassEntity, std::weak_ptr(grassMesh));

    //NPC
    m_entity_registry->emplace<TransformComponent>(m_npcEntity,
        glm::vec3{5.0f, 0.0f, -5.0f},
        0.0f,
        0.0f,
        // glm::vec3{0.01f, 0.01f, 0.01f});
        glm::vec3{0.03f, 0.03f, 0.03f});
    m_entity_registry->emplace<MeshComponent>(m_npcEntity, std::weak_ptr(horseMesh));
    m_entity_registry->emplace<NPCControllerComponent>(m_npcEntity,
        std::vector<glm::vec3>{ //NPC waypoints
        glm::vec3{5.0f,  0.0f,  0.0f},
        glm::vec3{10.0f, 0.0f, -10.0f}, 
        glm::vec3{30.0f, 0.0f, -10.0f}, 
        glm::vec3{30.0f, 0.0f, -35.0f}});
    m_entity_registry->emplace<AnimationComponent>(m_npcEntity,
        2, 4, 1.0f, 1.0f, true, 0.0f, std::string("mixamorig:Spine"));

    //POINT LIGHT
    m_entity_registry->emplace<PointLightComponent>(m_lightEntity,
        glm::vec3{0.0f, 5.0f, 0.0f},    //pos
        glm::vec3{1.0f, 1.0f, 1.0f});   //color
    
    return true;
}

void Game::update(
    float time,
    float deltaTime,
    InputManagerPtr input)
{
    // //core
    MovementSystem::Update(deltaTime, *m_entity_registry);
    AnimationSystem::Update(deltaTime, *m_entity_registry);

    // //game
    PlayerControllerSystem::Update(deltaTime, input, *m_entity_registry);
    // PlayerAnimationSystem::Update(*m_entity_registry);
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
    renderUI();
}

void Game::renderUI()
{
    // Begin game info ImGui window
    ImGui::Begin("Game Info");

    //PLAYER
    ImGui::Text("Drawcall count %i", drawcallCount);
    if (ImGui::CollapsingHeader("Player Settings")) {
        auto& playerTransform = *m_entity_registry->try_get<TransformComponent>(m_playerEntity);
        ImGui::DragFloat3("Scale##Player", &playerTransform.scale.x, 0.01f); 
        
        auto& cameraController = *m_entity_registry->try_get<ThirdPersonCameraControllerComponent>(m_cameraEntity);
        ImGui::SliderFloat("Camera Distance", &cameraController.distance, 0.0f, 500.0f);
        
        auto& playerController = *m_entity_registry->try_get<PlayerControllerComponent>(m_playerEntity);
        ImGui::DragFloat("Acceleration Rate", &playerController.acceleration_rate);   
        ImGui::DragFloat("Friction", &playerController.friction, 0.0f, 60);   

        auto& velocityComponent = *m_entity_registry->try_get<LinearVelocityComponent>(m_playerEntity);
        float velocity = glm::length(velocityComponent.velocity);
        ImGui::Text("Current velocity: %.1f m/s", velocity);        
    }
    if (ImGui::CollapsingHeader("Player Animation Settings")) {
        auto& animationController = *m_entity_registry->try_get<PlayerAnimationControllerComponent>(m_playerEntity);
        ImGui::SliderFloat("Velocity Threshold Walk", &animationController.thresholdWalk, 0.0f, 100.0f);
        ImGui::SliderFloat("Velocity Threshold Run", &animationController.thresholdRun, 0.0f, 100.0f);
        
        auto& animationComponent = *m_entity_registry->try_get<AnimationComponent>(m_playerEntity);
        ImGui::SliderFloat("Animation Blend", &animationComponent.blendFactor, 0.0f, 1.0f);
        ImGui::SliderInt("Animation Index A", &animationComponent.animIndexA, 0, 3);
        ImGui::SliderInt("Animation Index B", &animationComponent.animIndexB, 0, 3);
        ImGui::Checkbox("Use Layering", &animationComponent.useLayering);
    }
    //NPC
    if (ImGui::CollapsingHeader("NPC Settings")) {
        if (auto npcTransform = m_entity_registry->try_get<TransformComponent>(m_npcEntity)) {
            ImGui::DragFloat3("Scale##NPC", &npcTransform->scale.x, 0.01f); 
        }
        if(auto NPCController = m_entity_registry->try_get<NPCControllerComponent>(m_npcEntity)) {
            ImGui::SliderFloat("NPC Velocity Distance", &NPCController->velocity, 0.0f, 500.0f);
        }
    }
    //LIGHT
    if (ImGui::CollapsingHeader("Light Settings")) {
        auto* lightPtr = m_entity_registry->try_get<PointLightComponent>(m_lightEntity);
        if (lightPtr) {
            auto& lightRef = *lightPtr;
            if (ImGui::ColorEdit3("Light color", glm::value_ptr(lightRef.color),
            ImGuiColorEditFlags_NoInputs))
            {}
            ImGui::DragFloat3("Light Pos X", &lightRef.position.x, 0.1f);
        }
        else {
            ImGui::Text("No Light Found.");
        }
    }
    ImGui::Checkbox("Render Gizmos", &m_renderGizmos);

    ImGui::End(); // end info window
}

void Game::destroy()
{

}
