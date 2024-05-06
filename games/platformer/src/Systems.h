#pragma once

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include <edbr/ECS/Components/MovementComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>

#include "Components.h"
#include "EntityUtil.h"

// set direction based on velocity
inline void directionSystemUpdate(entt::registry& registry, float dt)
{
    for (auto&& [e, tc, mc] : registry.view<TransformComponent, MovementComponent>().each()) {
        if (mc.effectiveVelocity.x != 0.f || mc.effectiveVelocity.y != 0.f) {
            entityutil::setHeading2D({registry, e}, glm::vec2{mc.effectiveVelocity});
        }
    }
}

// TODO: do this in state machine
inline void playerAnimationSystemUpdate(entt::registry& registry, float dt)
{
    auto player = entityutil::getPlayerEntity(registry);
    auto& mc = player.get<MovementComponent>();
    if (mc.effectiveVelocity.x != 0.f || mc.effectiveVelocity.y != 0.f) {
        entityutil::setSpriteAnimation(player, "run");
    } else {
        entityutil::setSpriteAnimation(player, "idle");
    }
}

inline void spriteAnimationSystemUpdate(entt::registry& registry, float dt)
{
    for (const auto&& [e, tc, sc, ac] :
         registry.view<TransformComponent, SpriteComponent, SpriteAnimationComponent>().each()) {
        ac.animator.update(dt);
        ac.animator.animate(sc.sprite, ac.animationsData->getSpriteSheet());

        // flip on X if looking left
        // TODO: check if animation has is direction
        if (entityutil::getHeading2D({registry, e}).x < 0.f) {
            std::swap(sc.sprite.uv0.x, sc.sprite.uv1.x);
        }
    }
}
