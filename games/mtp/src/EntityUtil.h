#pragma once

#include <string>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include <fmt/format.h>

#include <edbr/ECS/Components/NameComponent.h>
#include <edbr/GameCommon/EntityUtil.h>

class EventManager;

namespace entityutil
{
// allows to send events from any function without carrying event manager around
void setEventManager(EventManager& eventManager);

void addChild(entt::handle parent, entt::handle child);
glm::vec3 getWorldPosition(entt::handle e);
glm::vec3 getLocalPosition(entt::handle e);
// can only be called on non-physics entities without parents
void setPosition(entt::handle e, const glm::vec3& pos);
// call this, will sync physics automatically
void teleportEntity(entt::handle e, const glm::vec3& pos);
void setRotation(entt::handle e, const glm::quat& rotation);
void rotateSmoothlyTo(entt::handle e, const glm::quat& targetHeading, float rotationTime);
void setAnimation(entt::handle e, const std::string& name);

// Find entity by glTF scene node name - pretty slow
entt::handle findEntityBySceneNodeName(entt::registry& registry, const std::string& name);

entt::handle findPlayerSpawnByName(entt::registry& registry, const std::string& name);
entt::handle findCameraByName(entt::registry& registry, const std::string& name);

entt::handle getPlayerEntity(entt::registry& registry);
bool playerExists(entt::registry& registry);
void spawnPlayer(entt::registry& registry, const std::string& spawnName);

template<typename ComponentType>
entt::handle findEntityByName(entt::registry& registry, const std::string& name)
{
    for (const auto& e : registry.view<ComponentType>()) {
        if (auto ncPtr = registry.try_get<NameComponent>(e); ncPtr && ncPtr->name == name) {
            return {registry, e};
        }
    }
    fmt::println("Entity with name '{}' was not found", name);
    return {registry, entt::null};
}

// returns names (if they exist) in this order
// 1. tag
// 2. name from NameComponent
// 3. scene node name
const std::string& getMetaName(entt::const_handle e);

}
