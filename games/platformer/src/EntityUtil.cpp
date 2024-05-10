#include "EntityUtil.h"

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MovementComponent.h>
#include <edbr/ECS/Components/PersistentComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>

#include "Components.h"

#include <fmt/printf.h>

namespace entityutil
{

math::FloatRect getAABB(entt::const_handle e)
{
    auto& cc = e.get<CollisionComponent>();
    auto origin = cc.origin;
    if (origin == glm::vec2{}) {
        origin = {-cc.size.x / 2.f, -cc.size.y};
    }
    const auto pos = getWorldPosition2D(e);
    const auto tl = pos + origin;
    return {tl, cc.size};
}

entt::handle getPlayerEntity(entt::registry& registry)
{
    for (const auto& e : registry.view<PlayerComponent>()) {
        return {registry, e};
    }
    return {};
}

void spawnPlayer(entt::registry& registry, const std::string& spawnName)
{
    auto player = getPlayerEntity(registry);
    auto spawn = getEntityByTag(registry, spawnName);
    assert(spawn.entity() != entt::null && "spawn was not found");

    // copy heading (don't copy scale)
    auto& playerTC = player.get<TransformComponent>();
    const auto& spawnTC = spawn.get<TransformComponent>();
    playerTC.transform.setHeading(spawnTC.transform.getHeading());

    setWorldPosition2D(player, getWorldPosition2D(spawn));
}

entt::handle findInteractableEntity(entt::registry& registry)
{
    const auto& player = getPlayerEntity(registry);
    const auto playerBB = getAABB(player);
    for (const auto& [e, cc, ic] : registry.view<CollisionComponent, InteractComponent>().each()) {
        const auto& eBB = getAABB({registry, e});
        if (eBB.intersects(playerBB)) {
            return {registry, e};
        }
    }
    return {};
}

void stopMovement(entt::handle e)
{
    auto& mc = e.get<MovementComponent>();
    mc.rotationProgress = mc.rotationTime;
    mc.rotationTime = 0.f;
    mc.kinematicVelocity = {};
}

}
