#include "glmcommon.hpp"
#include "RenderableMesh.hpp"
#include "CoreComponents.hpp"
#include "CollisionComponents.hpp"
#include "BVHSystems.hpp"
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
    static void Update(entt::registry& registry) {
        std::vector<std::pair<SphereColliderComponent, entt::entity>> entitySphereColliders;
        auto view = registry.view<SphereColliderComponent, AABBColliderComponent, TransformComponent>();

        // --- BROAD PHASE ---
        for(auto entity : view) {
            auto collider = view.get<SphereColliderComponent>(entity);
            entitySphereColliders.push_back(std::pair<SphereColliderComponent, entt::entity>(collider, entity));
        }
        float maxDistanceBetweenLeaves = 10.0f;
        auto treeRoot = BVHSystem::BuildBVHBottomUp(entitySphereColliders, maxDistanceBetweenLeaves);
        
        for(auto entityA : view) {
            auto sphereLooseA = view.get<SphereColliderComponent>(entityA);
            auto aabbTightA = view.get<AABBColliderComponent>(entityA);
            std::vector<entt::entity> possibleCollisions = BVHSystem::FindPossibleCollisions(treeRoot, sphereLooseA.GetSphere());
            for(auto entityB : possibleCollisions) {
                if (entityA == entityB) continue;
                if (entityB == entt::null) continue;
                
                //narrow phase: step 1 (loose)
                auto sphereLooseB = view.get<SphereColliderComponent>(entityB);
                if(TestSphereSphere(sphereLooseA, sphereLooseB)) {
                    eeng::Log("LOOSE COLLISION");
                
                    //narrow phase: step 2 (tight)
                    auto aabbTightB = view.get<AABBColliderComponent>(entityB);
                    if (TestAABBAABB(aabbTightA, aabbTightB)) {
                        eeng::Log("TIGHT COLLISION");
                        auto& tfmA = view.get<TransformComponent>(entityA);
                        SeparateAABBs_XZ(aabbTightA, aabbTightB, tfmA);
                    }
                }
            }
        }

        // --- NARROW PHASE ---
        // - loose
        // - tight

    }
    static void CheckCollisions(entt::registry& registry) {
        // CheckSphereCollisions(registry);
        // CheckAABBCollisions(registry);
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
        static const float separationModifier = 0.0314; 
        AABB boxA = a.aabb;
        AABB boxB = b.aabb;

        glm::vec3 u = {0, 0, boxA.max.z - boxB.min.z};  //up
        glm::vec3 d = {0, 0, boxA.min.z - boxB.max.z};  //down
        glm::vec3 l = {boxA.min.x - boxB.max.x, 0, 0};  //left
        glm::vec3 r = {boxA.max.x - boxB.min.x, 0, 0};  //right

        //find shortest length
        std::vector<float> lengths{u.z, d.z, l.x, r.x};
        std::sort(lengths.begin(), lengths.end());

        //put vecs in dictionary
        std::map<float, glm::vec3> vecs;
        vecs.insert_or_assign(u.z, u);
        vecs.insert_or_assign(d.z, d);
        vecs.insert_or_assign(l.x, l);
        vecs.insert_or_assign(r.x, r);

        //use shortest vector as separation
        glm::vec3 shortest = vecs[lengths[0]];
        if (!a.isStatic)
            tfmA.position += shortest * separationModifier;
    }
};