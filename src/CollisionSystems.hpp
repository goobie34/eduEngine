#include "glmcommon.hpp"
#include "RenderableMesh.hpp"
#include "CoreComponents.hpp"
#include "CollisionComponents.hpp"
#include <entt/entt.hpp>
#include "Log.hpp"



#pragma once

using namespace eeng;

class ColliderSystem {
public:
    static void UpdateAABBs(entt::registry& registry) {
        auto view = registry.view<TransformComponent, MeshComponent, AABBColliderComponent>();
        for(auto entity : view) {
            auto& collider = view.get<AABBColliderComponent>(entity);
            if (!collider.setFromMesh) return;

            auto& transform = view.get<TransformComponent>(entity);
            auto& meshComponent = view.get<MeshComponent>(entity);
            
            if (auto mesh = meshComponent.mesh.lock())
                collider.aabb = mesh->m_model_aabb.post_transform(transform.Matrix());
        }
    }
    static void UpdateSpheres(entt::registry& registry) {
        auto view = registry.view<TransformComponent, MeshComponent, SphereColliderComponent>();
        for(auto entity : view) {
            auto& collider = view.get<SphereColliderComponent>(entity);
            if (!collider.setFromMesh) return;
            
            auto& transform = view.get<TransformComponent>(entity);
            auto& meshComponent = view.get<MeshComponent>(entity);

            if (auto mesh = meshComponent.mesh.lock()) {
                glm::vec4 bs = mesh->m_model_aabb.post_transform(transform.Matrix()).getBoundingSphere();
                collider.pos = {bs.x, bs.y, bs.z};
                collider.radius = bs.w;
            }
        }
    }
};
class CollisionSystem {
public:
    static void CheckCollisions(entt::registry& registry) {
        // CheckSphereCollisions(registry);
        CheckAABBCollisions(registry);
    }

    static void CheckSphereCollisions(entt::registry& registry) {
        auto view = registry.view<TransformComponent, SphereColliderComponent>();
        for(auto entityA : view) {
            for(auto entityB : view) {
                if(entityA == entityB) continue;
                auto& sphereA = view.get<SphereColliderComponent>(entityA);
                auto& sphereB = view.get<SphereColliderComponent>(entityB);
                if(auto collision = TestSphereSphere(sphereA, sphereB)) {
                    collision->thisEntity = entityA;
                    collision->otherEntity = entityB;
                    // colliderA.OnCollision();
                    // colliderB.OnCollision();
                    SeparateSpheres(collision, registry);
                    eeng::Log("Collision occurred: sphere-sphere");
                }
            }
        }
    }

    static SimpleCollision* TestSphereSphere(SphereColliderComponent a, SphereColliderComponent b) {    
        glm::vec3 centerToCenter = a.pos - b.pos;
        float distanceSquared = glm::dot(centerToCenter, centerToCenter);

        float radiusSum = a.radius + b.radius;
        if (distanceSquared > radiusSum * radiusSum) return nullptr;

        SimpleCollision* collision = new SimpleCollision();
        collision->contactNormal = glm::normalize(centerToCenter);
        collision->contactPoint = a.pos + collision->contactNormal * a.radius;
        collision->penetrationDepth = radiusSum - glm::sqrt(distanceSquared);

        return collision;
    }

    static void CheckAABBCollisions(entt::registry& registry) {
        auto view = registry.view<TransformComponent, AABBColliderComponent>();
        for(auto entityA : view) {
            for(auto entityB : view) {
                if(entityA == entityB) continue;
                auto& colliderA = view.get<AABBColliderComponent>(entityA);
                auto& colliderB = view.get<AABBColliderComponent>(entityB);
                auto& tfmA = view.get<TransformComponent>(entityA);
                if(TestAABBAABB(colliderA, colliderB)) {
                    // colliderA.OnCollision();
                    // colliderB.OnCollision();
                    SeparateAABBs_XZ(colliderA, colliderB, tfmA);
                    eeng::Log("Collision occurred: AABB-AABB");
                }
            }
        }
    }
    static bool TestAABBAABB(AABBColliderComponent a, AABBColliderComponent b) {    
        return a.aabb.intersect(b.aabb);
    }

    static void SeparateSpheres(const SimpleCollision* collision, entt::registry& registry) {
        auto tfmA = registry.try_get<TransformComponent>(collision->thisEntity);
        auto tfmB = registry.try_get<TransformComponent>(collision->otherEntity);
        if (!tfmA || !tfmB) return;

        tfmA->position -= collision->contactNormal * (collision->penetrationDepth * 0.5f);
        tfmB->position += collision->contactNormal * (collision->penetrationDepth * 0.5f);
    }

    static void SeparateAABBs_XZ (AABBColliderComponent a, AABBColliderComponent b, TransformComponent& tfmA) {
        AABB boxA = a.aabb;
        AABB boxB = b.aabb;

        glm::vec3 u = {0, 0, boxA.max.z - boxB.min.z};
        glm::vec3 d = {0, 0, boxA.min.z - boxB.max.z};
        glm::vec3 l = {boxA.min.x - boxB.max.x, 0, 0};
        glm::vec3 r = {boxA.max.x - boxB.min.x, 0, 0};

        std::vector<float> lengths{u.z, d.z, l.x, r.x};
        std::sort(lengths.begin(), lengths.end());

        std::map<float, glm::vec3> vecs;
        vecs.insert_or_assign(u.z, u);
        vecs.insert_or_assign(d.z, d);
        vecs.insert_or_assign(l.x, l);
        vecs.insert_or_assign(r.x, r);

        glm::vec3 shortest = vecs[lengths[0]];
        tfmA.position += shortest;
    }
};