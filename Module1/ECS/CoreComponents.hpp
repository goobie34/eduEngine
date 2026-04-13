#include "glmcommon.hpp"
#include "RenderableMesh.hpp"

#pragma once

using namespace eeng;

struct TransformComponent {
    glm::vec3 position;
    glm::mat3 rotation;
    glm::vec3 scale;
};

struct LinearVelocityComponent {
    glm::vec3 velocity;
};

struct MeshComponent {
    std::weak_ptr<RenderableMesh> mesh;
};

struct PointLightComponent{
    glm::vec3 position;
    glm::vec3 color;
};

struct CameraComponent{
    CameraComponent(float fov, float nearPlane, float farPlane,
                    bool isMain, entt::entity lookAt = entt::null)
                    : fov(fov), nearPlane(nearPlane), farPlane(farPlane),
                    isMain(isMain), lookAtEntity(lookAt) {}

    float fov, nearPlane, farPlane;
    bool isMain;
    entt::entity lookAtEntity; //this should just be a vector3

    //matrices, initialized to identity
    //these are set in the camera system
    glm::mat4 projectionMatrix = glm::mat4(1.0);
    glm::mat4 viewMatrix = glm::mat4(1.0);
    glm::mat4 viewportMatrix = glm::mat4(1.0);
};