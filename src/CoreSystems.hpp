#include "glmcommon.hpp"
#include "RenderableMesh.hpp"
#include "CoreComponents.hpp"
#include "CollisionComponents.hpp"
#include <entt/entt.hpp>
#include <ForwardRenderer.hpp>
#include <ShapeRenderer.hpp>

#pragma once

using namespace eeng;
using namespace ShapeRendering;

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
                forwardRenderer->renderMesh(mesh_ptr, glm_aux::T(transform.position) * transform.RotationMatrix() * glm_aux::S(transform.scale));
            }
        }
        
    }
    static entt::entity getMainCam(entt::registry& registry) {
        //get all cameras
        auto camView = registry.view<CameraComponent, TransformComponent>();
        
        //search for main camera, give warning if more than one found
        entt::entity mainCamera = entt::null;
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

class GizmoSystem {
public:
    static void Render(ShapeRendererPtr shapeRenderer, entt::registry& registry) {
        PushBoneGizmo(shapeRenderer, registry);
        PushAABBs    (shapeRenderer, registry);
        PushSpheres  (shapeRenderer, registry);     
        RenderShapes (shapeRenderer, registry);
    }

    static void PushAABBs(ShapeRendererPtr shapeRenderer, entt::registry& registry) {
        auto view = registry.view<AABBColliderComponent, TransformComponent>();
        for(auto entity : view)
        {
            const AABBColliderComponent& collider = view.get<AABBColliderComponent>(entity);
            const TransformComponent& transform = view.get<TransformComponent>(entity);
            shapeRenderer->push_states(collider.isTrigger ? ShapeRendering::Color4u::Yellow : ShapeRendering::Color4u::Red);
            shapeRenderer->push_AABB(collider.aabb.min, collider.aabb.max);
            shapeRenderer->pop_states<ShapeRendering::Color4u>();
        }
    }

    static void PushSpheres(ShapeRendererPtr shapeRenderer, entt::registry& registry) {
        auto view = registry.view<SphereColliderComponent, TransformComponent>();
        for(auto entity : view) {
            auto& sphereCollider = view.get<SphereColliderComponent>(entity);
            glm::mat4 M = glm::translate(glm::mat4(1.0f), sphereCollider.pos);            

            shapeRenderer->push_states(sphereCollider.isTrigger ? ShapeRendering::Color4u::Yellow : ShapeRendering::Color4u::Blue);
            shapeRenderer->push_states(M);
            shapeRenderer->push_sphere_wireframe(sphereCollider.radius, sphereCollider.radius);
            shapeRenderer->pop_states<glm::mat4>();
            shapeRenderer->pop_states<ShapeRendering::Color4u>();
        }
    }

    static void PushBoneGizmo(ShapeRendererPtr shapeRenderer, entt::registry& registry) {
        float axisLen = 10.0f;

        auto meshView = registry.view<TransformComponent, MeshComponent>();
        for(auto entity : meshView)
        {
            const std::weak_ptr mesh = meshView.get<MeshComponent>(entity).mesh;
            const TransformComponent& transform = meshView.get<TransformComponent>(entity);

            if(auto mesh_ptr = mesh.lock()) //gets shared_ptr from weak_ptr, which is then released when scope ends
            {
                glm::mat4 transformationMatrix = glm_aux::T(transform.position) * transform.RotationMatrix() * glm_aux::S(transform.scale);
                for (int i = 0; i < mesh_ptr->boneMatrices.size(); ++i) {
                    auto IBinverse = glm::inverse(mesh_ptr->m_bones[i].inversebind_tfm);
                    glm::mat4 global = transformationMatrix * mesh_ptr->boneMatrices[i] * IBinverse;
                    glm::vec3 pos = glm::vec3(global[3]);
                    
                    glm::vec3 right = glm::vec3(global[0]); // X
                    glm::vec3 up    = glm::vec3(global[1]); // Y
                    glm::vec3 fwd   = glm::vec3(global[2]); // Z

                    shapeRenderer->push_states(ShapeRendering::Color4u::Red);
                    shapeRenderer->push_line(pos, pos + axisLen * right);

                    shapeRenderer->push_states(ShapeRendering::Color4u::Green);
                    shapeRenderer->push_line(pos, pos + axisLen * up);

                    shapeRenderer->push_states(ShapeRendering::Color4u::Blue);
                    shapeRenderer->push_line(pos, pos + axisLen * fwd);

                    shapeRenderer->pop_states<ShapeRendering::Color4u>();
                    shapeRenderer->pop_states<ShapeRendering::Color4u>();
                    shapeRenderer->pop_states<ShapeRendering::Color4u>();
                }
            }
        }
    }

    static void RenderShapes(ShapeRendererPtr shapeRenderer, entt::registry& registry) {
        entt::entity cam_entity = RenderSystem::getMainCam(registry);
        assert(cam_entity != entt::null && "GizmoSystem: Cannot find main camera.");
        CameraComponent camera = registry.get<CameraComponent>(cam_entity);
        shapeRenderer->render(camera.projectionMatrix * camera.viewMatrix);
        shapeRenderer->post_render();
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
                        
            if (camera.isPivot) {
                camera.viewMatrix = glm::lookAt(transform.position, camera.lookAt_pos, glm::vec3{0.0f,1.0f,0.0f});
            }
            else
            {
                camera.viewMatrix = glm::transpose(transform.RotationMatrix())
                * glm_aux::T(-1.0f * (transform.position));

            }
        }
    }
};

class AnimationSystem {
public:
    static void Update(float dt, entt::registry& registry) {
        auto view = registry.view<AnimationComponent, MeshComponent>();

        for(auto entity : view)
        {
            auto& animationComponent = view.get<AnimationComponent>(entity);
            std::weak_ptr<RenderableMesh> mesh = view.get<MeshComponent>(entity).mesh;
            animationComponent.time += dt;

            if(auto mesh_ptr = mesh.lock()) //gets shared_ptr from weak_ptr, which is then released when scope ends
            {
                if (animationComponent.useLayering)
                {
                    eeng::AnimationBranchDesc upperBodyFilter;
                    upperBodyFilter.root_node_name = animationComponent.subTreeRootNode;
                    upperBodyFilter.mode = eeng::AnimationBranchDesc::Mode::IncludeSubtree;
                    mesh_ptr->animateBlend(animationComponent.animIndexA, animationComponent.animIndexB,
                                           animationComponent.time, animationComponent.time,
                                           upperBodyFilter);
                }
                else
                {
                    mesh_ptr->animateBlend(animationComponent.animIndexA, animationComponent.animIndexB,
                                           animationComponent.time, animationComponent.time,
                                           animationComponent.blendFactor);
                }
            }
        }
    }    
};
