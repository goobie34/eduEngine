#ifndef Game_hpp
#define Game_hpp
#pragma once

#include <entt/fwd.hpp>
#include "GameBase.h"
#include "RenderableMesh.hpp"
#include "ForwardRenderer.hpp"
#include "ShapeRenderer.hpp"

/// @brief A Game may hold, update and render 3D geometry and GUI elements
class Game : public eeng::GameBase
{
public:
    /// @brief For game resource initialization
    /// @return 
    bool init() override;

    /// @brief General update method that is called each frame
    /// @param time Total time elapsed in seconds
    /// @param deltaTime Time elapsed since the last frame
    /// @param input Input from mouse, keyboard and controllers
    void update(
        float time,
        float deltaTime,
        InputManagerPtr input) override;

    /// @brief For rendering of game contents
    /// @param time Total time elapsed in seconds
    /// @param screenWidth Current width of the window in pixels
    /// @param screenHeight Current height of the window in pixels
    void render(
        float time,
        int windowWidth,
        int windowHeight) override;

    /// @brief For destruction of game resources
    void destroy() override;

private:
    /// @brief For rendering of GUI elements
    void renderUI();

    // Renderer for rendering imported animated or non-animated models
    eeng::ForwardRendererPtr forwardRenderer;

    // Immediate-mode renderer for basic 2D or 3D primitives
    ShapeRendererPtr shapeRenderer;
    bool m_renderGizmos = false;

    // Entities
    entt::entity m_playerEntity;
    entt::entity m_npcEntity;
    entt::entity m_npcProximityTriggerEntity;
    entt::entity m_itemSpawner;
    entt::entity m_environmentEntity;
    entt::entity m_cameraEntity;
    entt::entity m_lightEntity;
    entt::entity m_guiEntity;

    // Game meshes
    std::shared_ptr<eeng::RenderableMesh> m_environmentMesh, m_npcMesh, m_characterMesh, m_itemMesh;

    //AABBs
    AABB m_playerAABB, m_npcAABB, m_environmentAABB;

    // Stats
    int drawcallCount = 0;
};

#endif
