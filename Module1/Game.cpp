#include <entt/entt.hpp>
#include "glmcommon.hpp"
#include "imgui.h"
#include "Game.hpp"
#include "ECS/CoreComponents.hpp"
#include "ECS/CoreSystems.hpp"
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
    horseMesh->load("assets/Animals/Horse.fbx", false);

    characterMesh = std::make_shared<eeng::RenderableMesh>();
    characterMesh->load("assets/Amy/Ch46_nonPBR.fbx");
    characterMesh->load("assets/Amy/idle.fbx", true);
    characterMesh->load("assets/Amy/walking.fbx", true);
    characterMesh->load("assets/Amy/waving.fbx", true);
    characterMesh->removeTranslationKeys("mixamorig:Hips");
    
    //Set up EnTT
    entity_registry = std::make_shared<entt::registry>();

    //Create entities
    m_playerEntity = entity_registry->create();
    m_cameraEntity = entity_registry->create();
    m_grassEntity = entity_registry->create();
    m_npcEntity = entity_registry->create();
    m_lightEntity = entity_registry->create();

    //Assign components to entities
    //PLAYER
    entity_registry->emplace<TransformComponent>(m_playerEntity,
        glm_aux::vec3_000,
        glm::mat3(1.0),
        glm::vec3{0.03f, 0.03f, 0.03f});
    entity_registry->emplace<MeshComponent>(m_playerEntity, std::weak_ptr(characterMesh));
    entity_registry->emplace<ThirdPersonCameraControllerComponent>(m_playerEntity, m_cameraEntity);
    entity_registry->emplace<LinearVelocityComponent>(m_playerEntity, LinearVelocityComponent{});
    using Key = InputManager::Key;
    entity_registry->emplace<PlayerControllerComponent>(
        m_playerEntity, Key::W, Key::A, Key::S, Key::D, Key::LeftShift);

    //CAMERA
    entity_registry->emplace<TransformComponent>(m_cameraEntity,
        glm_aux::vec3_000,
        glm::mat3(1.0),
        glm::vec3{1, 1, 1});
    entity_registry->emplace<CameraComponent>(m_cameraEntity,
        60.0f,  //fov
        1.0f,   //near
        500.0f, //far
        true,   //isMain
        m_playerEntity);    // lookAt entity

    //ENVIRONMENT
    entity_registry->emplace<TransformComponent>(m_grassEntity,
        glm::vec3{0.0f, 0.0f, 0.0f},
        glm::mat3(1.0),
        glm::vec3{100.0f, 100.0f, 100.0f});
    entity_registry->emplace<MeshComponent>(m_grassEntity, std::weak_ptr(grassMesh));

    //NPC
    entity_registry->emplace<TransformComponent>(m_npcEntity,
        glm::vec3{30.0f, 0.0f, -35.0f},
        glm::mat3(1.0),
        glm::vec3{0.01f, 0.01f, 0.01f});
    entity_registry->emplace<MeshComponent>(m_npcEntity, std::weak_ptr(horseMesh));
    entity_registry->emplace<NPCControllerComponent>(m_npcEntity,
        std::vector<glm::vec3>{ //NPC waypoints
        glm::vec3{10.0f, 0.0f, -35.0f},
        glm::vec3{10.0f, 0.0f, -10.0f}, 
        glm::vec3{30.0f, 0.0f, -10.0f}, 
        glm::vec3{30.0f, 0.0f, -35.0f}});

    //POINT LIGHT
    entity_registry->emplace<PointLightComponent>(m_lightEntity,
        glm::vec3{0.0f, 5.0f, 0.0f},    //pos
        glm::vec3{1.0f, 1.0f, 1.0f});   //color

    // //DEBUG NPC
    for(int i = 0; i < 999; i++) {
        entt::entity debugNpc = entity_registry->create();
        entity_registry->emplace<TransformComponent>(debugNpc,
            glm::vec3{30.0f + i, 0.0f, -35.0f + i},
            glm::mat3(1.0),
            glm::vec3{0.01f, 0.01f, 0.01f});
        entity_registry->emplace<MeshComponent>(debugNpc, std::weak_ptr(horseMesh));
        entity_registry->emplace<NPCControllerComponent>(debugNpc,
            std::vector<glm::vec3>{ //NPC waypoints
            glm::vec3{10.0f + i, 0.0f, -35.0f + i},
            glm::vec3{10.0f + i, 0.0f, -10.0f + i}, 
            glm::vec3{30.0f + i, 0.0f, -10.0f + i}, 
            glm::vec3{30.0f + i, 0.0f, -35.0f + i}});   
    }
    
    return true;
}

void Game::update(
    float time,
    float deltaTime,
    InputManagerPtr input)
{

    MovementSystem::Update(deltaTime, *entity_registry);
    PlayerControllerSystem::Update(input, *entity_registry);
    ThirdPersonCameraControllerSystem::Update(input, *entity_registry);
    NPCControllerSystem::Update(deltaTime, *entity_registry);
}

void Game::render(
    float time,
    int windowWidth,
    int windowHeight)
{
    CameraSystem::Update(windowWidth, windowHeight, *entity_registry);
    RenderSystem::Render(forwardRenderer, *entity_registry);
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
        auto& playerTransform = *entity_registry->try_get<TransformComponent>(m_playerEntity);
        ImGui::SliderFloat("Player Scale X", &playerTransform.scale.x, 0.0f, 1.0f);
        ImGui::SliderFloat("Player Scale Y", &playerTransform.scale.y, 0.0f, 1.0f);
        ImGui::SliderFloat("Player Scale Z", &playerTransform.scale.z, 0.0f, 1.0f);
        auto& cameraController = *entity_registry->try_get<ThirdPersonCameraControllerComponent>(m_playerEntity);
        ImGui::SliderFloat("Camera Distance", &cameraController.distance, 0.0f, 500.0f);
    }
    //NPC
    if (ImGui::CollapsingHeader("NPC Settings")) {
        auto& npcTransform = *entity_registry->try_get<TransformComponent>(m_npcEntity);
        ImGui::SliderFloat("NPC Scale X", &npcTransform.scale.x, 0.0f, 1.0f);
        ImGui::SliderFloat("NPC Scale Y", &npcTransform.scale.y, 0.0f, 1.0f);
        ImGui::SliderFloat("NPC Scale Z", &npcTransform.scale.z, 0.0f, 1.0f);
        auto& NPCController = *entity_registry->try_get<NPCControllerComponent>(m_npcEntity);
        ImGui::SliderFloat("NPC Velocity Distance", &NPCController.velocity, 0.0f, 500.0f);
    }
    //LIGHT
    if (ImGui::CollapsingHeader("Light Settings")) {
        auto* lightPtr = entity_registry->try_get<PointLightComponent>(m_lightEntity);
        if (lightPtr) {
            auto& lightRef = *lightPtr;
            if (ImGui::ColorEdit3("Light color", glm::value_ptr(lightRef.color),
            ImGuiColorEditFlags_NoInputs))
            {}
            ImGui::DragFloat("Light Pos X", &lightRef.position.x);
            ImGui::DragFloat("Light Pos Y", &lightRef.position.y);
            ImGui::DragFloat("Light Pos Z", &lightRef.position.z);
        }
        else {
            ImGui::Text("No Light Found.");
        }
    }

    ImGui::End(); // end info window
}

void Game::destroy()
{

}
