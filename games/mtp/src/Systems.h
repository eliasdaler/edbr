#pragma once

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include "Components.h"
#include "EntityUtil.h"
#include <edbr/ECS/Components/HierarchyComponent.h>

namespace eu = entityutil;

inline void movementSystemUpdate(entt::registry& registry, float dt)
{
    for (auto&& [e, tc, mc] : registry.view<TransformComponent, MovementComponent>().each()) {
        const auto newPos = tc.transform.getPosition() + mc.velocity * dt;
        tc.transform.setPosition(newPos);
    }
}

namespace
{
void updateEntityTransforms(
    entt::registry& registry,
    entt::entity e,
    const glm::mat4& parentWorldTransform)
{
    auto [tc, hc] = registry.get<TransformComponent, const HierarchyComponent>(e);
    if (hc.hasParent()) {
        tc.worldTransform = tc.transform.asMatrix();
    } else {
        const auto prevTransform = tc.worldTransform;
        tc.worldTransform = parentWorldTransform * tc.transform.asMatrix();
        if (tc.worldTransform == prevTransform) {
            return;
        }
    }

    for (const auto& child : hc.children) {
        updateEntityTransforms(registry, child, tc.worldTransform);
    }
}
} // end of anonymous namespace

inline void transformSystemUpdate(entt::registry& registry, float dt)
{
    static const auto I = glm::mat4{1.f};
    for (auto&& [e, hc] : registry.view<HierarchyComponent>().each()) {
        if (!hc.hasParent()) { // start from root nodes
            updateEntityTransforms(registry, e, I);
        }
    }
}

inline void skeletonAnimationSystemUpdate(entt::registry& registry, float dt)
{
    // setting animation based on velocity
    // TODO: do this in state machine instead
    for (auto&& [e, mc, sc] : registry.view<MovementComponent, SkeletonComponent>().each()) {
        const auto velMag = glm::length(mc.velocity);
        if (std::abs(velMag) <= 0.001f) {
            const auto& animator = sc.skeletonAnimator;
            if (animator.getCurrentAnimationName() == "Run" ||
                animator.getCurrentAnimationName() == "Walk") {
                eu::setAnimation({registry, e}, "Idle");
            }
            continue;
        }

        if (mc.velocity.x != 0.f && mc.velocity.z != 0.f) {
            if (velMag > 2.f) {
                eu::setAnimation({registry, e}, "Run");
            } else if (velMag > 0.1f) {
                // HACK: sometimes the player can fall slightly when being spawned
                eu::setAnimation({registry, e}, "Walk");
            }
        }
    }

    // animate entities with skeletons
    for (const auto&& [e, sc] : registry.view<SkeletonComponent>().each()) {
        sc.skeletonAnimator.update(sc.skeleton, dt);
    }
}
