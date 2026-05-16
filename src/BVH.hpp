// #include "glmcommon.hpp"
// #include "AABB.h"
// #include "CollisionComponents.hpp"
// #include <entt/entt.hpp>

// #pragma once

// float DistanceBetweenSpheres(glm::vec4 leftSphere, glm::vec4 rightSphere) {
//     float centerDistance = (glm::vec3(rightSphere) - glm::vec3(leftSphere)).length();
//     float surfaceDistance = centerDistance - (leftSphere.w + rightSphere.w);
//     return std::max(0.0f, surfaceDistance);
// }

// void FindMinMaxPoints(glm::vec4 leftSphere, glm::vec4 rightSphere, glm::vec3 &minOut, glm::vec3 &maxOut){
//     glm::vec3 leftCenter = glm::vec3(leftSphere);
//     glm::vec3 rightCenter = glm::vec3(rightSphere);
//     float leftRadius = leftSphere.w;
//     float rightRadius = rightSphere.w;

//     minOut.x = std::min(leftCenter.x - leftRadius, rightCenter.x - rightRadius);
//     maxOut.x = std::max(leftCenter.x + leftRadius, rightCenter.x + rightRadius);

//     minOut.y = std::min(leftCenter.y - leftRadius, rightCenter.y - rightRadius);
//     maxOut.y = std::max(leftCenter.y + leftRadius, rightCenter.y + rightRadius);

//     minOut.z = std::min(leftCenter.z - leftRadius, rightCenter.z - rightRadius);
//     maxOut.z = std::max(leftCenter.z + leftRadius, rightCenter.z + rightRadius);
// }

// struct SphereNode {
//     glm::vec4 thisSphere;
//     entt::entity thisEntity;
//     SphereNode* leftChild;
//     SphereNode* rightChild;
// };

// SphereNode* BuildNodeFromSingleSphere(glm::vec4 sphere, entt::entity entity) {
    
//     return new SphereNode{sphere, entity, nullptr, nullptr};
// }

// SphereNode* BuildNodeFromSpheres(glm::vec4 leftSphere, glm::vec4 rightSphere) {
//     glm::vec3 minPoint, maxPoint;
    
//     FindMinMaxPoints(leftSphere, rightSphere, minPoint, maxPoint);

//     glm::vec3 midPoint = midPoint + (maxPoint - midPoint) * 0.5f;
//     float radius = (maxPoint - minPoint).length() * 0.5f;

//     return new SphereNode{glm::vec4(midPoint, radius), entt::null, nullptr, nullptr};
// }

// std::vector<std::pair<SphereNode*, SphereNode*>> FindPairs(std::vector<SphereNode*> openList, float maxDistance) {
//     std::vector<std::pair<SphereNode*, SphereNode*>> allPairs;
//     std::vector<SphereNode*> availableSpheres = openList;

//     while(!availableSpheres.empty()) {
//         SphereNode* current = availableSpheres.back();

//         float closestDistance = maxDistance;
//         SphereNode* bestMatch = nullptr;
//         int bestIndex = -1;

//         for(int j = 0; j < availableSpheres.size(); j++) {
//             float distance = DistanceBetweenSpheres(current->thisSphere, availableSpheres[j]->thisSphere);
//             if (distance < closestDistance) {
//                 closestDistance = distance;
//                 bestMatch = availableSpheres[j];
//                 bestIndex = j;
//             }
//         }

//         if (bestMatch) {
//             availableSpheres.erase(availableSpheres.begin() + bestIndex);
//         }

//         allPairs.push_back({current, bestMatch});
//     }

// }

// SphereNode* BuildBVHBottomUp(std::vector<std::pair<SphereColliderComponent, entt::entity>> entitySphereColliders, float maxDistanceBetweenLeaves) {
//     if (entitySphereColliders.size() == 0) return nullptr;

//     std::vector<SphereNode*> openList;
//     for (auto sphereCollider : entitySphereColliders) {
//         openList.push_back(BuildNodeFromSingleSphere(sphereCollider.first.GetSphere(), sphereCollider.second));
//     }

//     while(openList.size() != 1) {
//         auto pairs = FindPairs(openList, maxDistanceBetweenLeaves);
//             openList.clear();
//             for(auto pair : pairs) {
//                 if(pair.second) {
//                     auto node = BuildNodeFromSpheres(pair.first->thisSphere, pair.second->thisSphere);
//                     node->leftChild = pair.first;
//                     node->rightChild = pair.second;
//                     openList.push_back(node);
//                 }
//                 else {
//                     auto node = BuildNodeFromSingleSphere(pair.first->thisSphere, entt::null);
//                     node->leftChild = pair.first;
//                     openList.push_back(node);
//                 }
//             }
//         maxDistanceBetweenLeaves = std::numeric_limits<float>::max();
//     }
//     return openList[0];
// }

// std::vector<entt::entity> FindPossibleCollisions(SphereNode* treeRoot, glm::vec4 sphere) {
//     std::vector<entt::entity> possibleCollisions;

//     if (!treeRoot) return possibleCollisions;

//     if (!TestSphere(treeRoot->thisSphere))
// }