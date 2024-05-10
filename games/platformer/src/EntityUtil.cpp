#include "EntityUtil.h"

#include <edbr/ECS/Components/CollisionComponent2D.h>
#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MovementComponent.h>
#include <edbr/ECS/Components/PersistentComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>

#include "Components.h"

#include <fmt/printf.h>

namespace entityutil
{

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
    const auto playerBB = getCollisionAABB(player);
    for (const auto& [e, cc, ic] :
         registry.view<CollisionComponent2D, InteractComponent>().each()) {
        const auto& eBB = getCollisionAABB({registry, e});
        if (eBB.intersects(playerBB)) {
            return {registry, e};
        }
    }
    return {};
}
}
