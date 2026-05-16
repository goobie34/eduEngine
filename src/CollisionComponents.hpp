#include "glmcommon.hpp"
#include "AABB.h"
#include <entt/entt.hpp>

#pragma once

struct AABBColliderComponent { //add dynamic/static flag
    bool isTrigger = false;
    bool isStatic = false;
    bool setFromMesh = true;
    eeng::AABB aabb;
    std::function<void()> OnCollision = [] () { return;};
};
struct SphereColliderComponent {    
    bool isTrigger = false;
    bool isStatic = false;
    bool setFromMesh = true;
    glm::vec3 pos;
    float radius;
    std::function<void()> OnCollision = [] () { return;};
    glm::vec4 GetSphere() {return glm::vec4(pos, radius);}
};

struct SimpleCollision {
    entt::entity thisEntity;
    entt::entity otherEntity;
    float penetrationDepth;
    glm::vec3 contactPoint;
    glm::vec3 contactNormal;
};