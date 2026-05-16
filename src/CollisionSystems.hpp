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

class BVHSystem {
private:
    struct SphereNode {
        glm::vec4 thisSphere;
        entt::entity thisEntity;
        SphereNode* leftChild;
        SphereNode* rightChild;
    };
    
public:
//BVH
    static SphereNode* BuildNodeFromSingleSphere(glm::vec4 sphere, entt::entity entity) {
        
        return new SphereNode{sphere, entity, nullptr, nullptr};
    }

    static SphereNode* BuildNodeFromSpheres(glm::vec4 leftSphere, glm::vec4 rightSphere) {
        glm::vec3 minPoint, maxPoint;
        
        FindMinMaxPoints(leftSphere, rightSphere, minPoint, maxPoint);

        glm::vec3 midPoint = midPoint + (maxPoint - midPoint) * 0.5f;
        float radius = (maxPoint - minPoint).length() * 0.5f;

        return new SphereNode{glm::vec4(midPoint, radius), entt::null, nullptr, nullptr};
    }

    static std::vector<std::pair<SphereNode*, SphereNode*>> FindPairs(std::vector<SphereNode*> openList, float maxDistance) {
        std::vector<std::pair<SphereNode*, SphereNode*>> allPairs;
        std::vector<SphereNode*> availableSpheres = openList;

        while(!availableSpheres.empty()) {
            SphereNode* current = availableSpheres.back();

            float closestDistance = maxDistance;
            SphereNode* bestMatch = nullptr;
            int bestIndex = -1;

            for(int j = 0; j < availableSpheres.size(); j++) {
                float distance = DistanceBetweenSpheres(current->thisSphere, availableSpheres[j]->thisSphere);
                if (distance < closestDistance) {
                    closestDistance = distance;
                    bestMatch = availableSpheres[j];
                    bestIndex = j;
                }
            }

            if (bestMatch) {
                availableSpheres.erase(availableSpheres.begin() + bestIndex);
            }

            allPairs.push_back({current, bestMatch});
        }
    }

static SphereNode* BuildBVHBottomUp(std::vector<std::pair<SphereColliderComponent, entt::entity>> entitySphereColliders, float maxDistanceBetweenLeaves) {
    if (entitySphereColliders.size() == 0) return nullptr;

    std::vector<SphereNode*> openList;
    for (auto sphereCollider : entitySphereColliders) {
        openList.push_back(BuildNodeFromSingleSphere(sphereCollider.first.GetSphere(), sphereCollider.second));
    }

    while(openList.size() != 1) {
        auto pairs = FindPairs(openList, maxDistanceBetweenLeaves);
            openList.clear();
            for(auto pair : pairs) {
                if(pair.second) {
                    auto node = BuildNodeFromSpheres(pair.first->thisSphere, pair.second->thisSphere);
                    node->leftChild = pair.first;
                    node->rightChild = pair.second;
                    openList.push_back(node);
                }
                else {
                    auto node = BuildNodeFromSingleSphere(pair.first->thisSphere, entt::null);
                    node->leftChild = pair.first;
                    openList.push_back(node);
                }
            }
        maxDistanceBetweenLeaves = std::numeric_limits<float>::max();
    }
    return openList[0];
}

static std::vector<entt::entity> FindPossibleCollisions(SphereNode* treeRoot, glm::vec4 sphere) {
    std::vector<entt::entity> possibleCollisions;

    if (!treeRoot) return possibleCollisions;

    if (!TestSphere(treeRoot->thisSphere))
}

//helpers
    static float DistanceBetweenSpheres(glm::vec4 leftSphere, glm::vec4 rightSphere) {
        float centerDistance = (glm::vec3(rightSphere) - glm::vec3(leftSphere)).length();
        float surfaceDistance = centerDistance - (leftSphere.w + rightSphere.w);
        return std::max(0.0f, surfaceDistance);
    }

    static void FindMinMaxPoints(glm::vec4 leftSphere, glm::vec4 rightSphere, glm::vec3 &minOut, glm::vec3 &maxOut){
        glm::vec3 leftCenter = glm::vec3(leftSphere);
        glm::vec3 rightCenter = glm::vec3(rightSphere);
        float leftRadius = leftSphere.w;
        float rightRadius = rightSphere.w;

        minOut.x = std::min(leftCenter.x - leftRadius, rightCenter.x - rightRadius);
        maxOut.x = std::max(leftCenter.x + leftRadius, rightCenter.x + rightRadius);

        minOut.y = std::min(leftCenter.y - leftRadius, rightCenter.y - rightRadius);
        maxOut.y = std::max(leftCenter.y + leftRadius, rightCenter.y + rightRadius);

        minOut.z = std::min(leftCenter.z - leftRadius, rightCenter.z - rightRadius);
        maxOut.z = std::max(leftCenter.z + leftRadius, rightCenter.z + rightRadius);
    }
}