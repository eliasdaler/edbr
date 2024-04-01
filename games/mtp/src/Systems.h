#pragma once

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include "Components.h"
#include <edbr/ECS/Components/HierarchyComponent.h>

inline void movementSystemUpdate(entt::registry& registry, float dt)
{
    for (auto&& [e, tc, mc] : registry.view<TransformComponent, MovementComponent>().each()) {
        const auto newPos = tc.transform.getPosition() + mc.velocity * dt;
        tc.transform.setPosition(newPos);

        if (mc.rotationTime != 0.f) {
            mc.rotationProgress += dt;
            if (mc.rotationProgress >= mc.rotationTime) {
                tc.transform.setHeading(mc.targetHeading);
                mc.rotationProgress = mc.rotationTime;
                mc.rotationTime = 0.f;
                continue;
            }

            const auto newHeading = glm::
                slerp(mc.startHeading, mc.targetHeading, mc.rotationProgress / mc.rotationTime);
            tc.transform.setHeading(newHeading);
        }
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
    if (!hc.hasParent()) {
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
    // animate entities with skeletons
    for (const auto&& [e, sc] : registry.view<SkeletonComponent>().each()) {
        sc.skeletonAnimator.update(sc.skeleton, dt);
    }
}
