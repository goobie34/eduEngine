#include "glmcommon.hpp"
#include "RenderableMesh.hpp"
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

class RenderSystem {
public:
    static void Render(eeng::ForwardRendererPtr forwardRenderer, entt::registry& registry)
    {   
        entt::entity cam_id = getMainCam(registry);
        const glm::vec3& cam_pos = registry.get<TransformComponent>(cam_id).position;
        const CameraComponent& camera = registry.get<CameraComponent>(cam_id);

        //hard coded, 1 light
        //TODO: make 1 rendering pass per light source
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
    static entt::entity getMainCam(entt::registry& registry) {
        //get all cameras
        auto camView = registry.view<CameraComponent, TransformComponent>();
        
        //search for main camera, give warning if more than one found
        entt::entity mainCamera;
        bool mainCamFound = false;
        for(auto entity : camView) {
            if (auto camera = registry.try_get<CameraComponent>(entity)) {
                if (!camera->isMain) continue;
                if (mainCamFound) {
                    //THROW WARNING, MORE THAN ONE MAIN CAMERA FOUND
                    break;
                }

                mainCamera = entity;
                mainCamFound = true;
            }
        }
        return mainCamera;
    }
};

class CameraSystem {
public:
    static void Update(int windowWidth, int windowHeight, entt::registry& registry)
    {        
        auto view = registry.view<TransformComponent, CameraComponent>();
        for(auto entity : view)
        {
            CameraComponent& camera = view.get<CameraComponent>(entity);
            const TransformComponent& transform = view.get<TransformComponent>(entity);
            
            //these three should ideally not be recalculated every frame
            const float aspectRatio = float(windowWidth) / windowHeight;
            camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), aspectRatio, camera.nearPlane, camera.farPlane);
            camera.viewportMatrix = glm_aux::create_viewport_matrix(0.0f, 0.0f, windowWidth, windowHeight, 0.0f, 1.0f);
                        
            if (camera.lookAtEntity == entt::null) {
                camera.viewMatrix = glm::mat4(glm::transpose(transform.rotation))
                * glm_aux::T(-1.0f * (transform.position));
            }
            else if (auto lookAtTransform = registry.try_get<TransformComponent>(camera.lookAtEntity))
            {
                camera.viewMatrix = glm::lookAt(transform.position, lookAtTransform-> , glm::vec3{0.0f,1.0f,0.0f});
            }
            else
            {
                //TODO: throw error
            }
        }
    }
};

