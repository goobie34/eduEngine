#include "glmcommon.hpp"
#include "AABB.h"
#include <entt/entt.hpp>

#pragma once

struct SimpleCollision {
    entt::entity thisEntity = entt::null;
    entt::entity otherEntity = entt::null;
    float penetrationDepth = 0.0f;
    glm::vec3 contactPoint = glm_aux::vec3_000;
    glm::vec3 contactNormal = glm_aux::vec3_000;
};

struct AABBColliderComponent {
    bool isTrigger = false;
    bool isStatic = false;
    bool setFromMesh = true;
    eeng::AABB aabb;
    std::function<void(SimpleCollision)> OnCollision = [](SimpleCollision s) {return;};
};
struct SphereColliderComponent {    
    bool isTrigger = false;
    bool isStatic = false;
    bool setFromMesh = true;
    glm::vec3 pos;
    float radius;
    std::function<void(SimpleCollision)> OnCollision = [](SimpleCollision s) {return;};
    glm::vec4 GetSphere() {return glm::vec4(pos, radius);}
};