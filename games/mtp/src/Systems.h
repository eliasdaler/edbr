#pragma once

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/Event/EventManager.h>

#include "Components.h"
#include "EntityUtil.h"
#include "Events.h"
#include "PhysicsSystem.h"

inline void movementSystemUpdate(entt::registry& registry, float dt)
{
    for (auto&& [e, tc, mc] : registry.view<TransformComponent, MovementComponent>().each()) {
        mc.prevFramePosition = entityutil::getWorldPosition({registry, e});
        const auto newPos = tc.transform.getPosition() + mc.kinematicVelocity * dt;
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

inline void movementSystemPostPhysicsUpdate(entt::registry& registry, float dt)
{
    for (auto&& [e, tc, mc] : registry.view<TransformComponent, MovementComponent>().each()) {
        auto newPos = entityutil::getWorldPosition({registry, e});
        mc.effectiveVelocity = (newPos - mc.prevFramePosition) / dt;
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

inline void skeletonAnimationSystemUpdate(entt::registry& registry, EventManager& em, float dt)
{
    // animate entities with skeletons
    for (const auto&& [e, sc] : registry.view<SkeletonComponent>().each()) {
        sc.skeletonAnimator.update(sc.skeleton, dt);

        // send frame events
        if (sc.skeletonAnimator.hasFrameChanged()) {
            const auto& animation = *sc.skeletonAnimator.getAnimation();
            const auto& events = animation.getEventsForFrame(sc.skeletonAnimator.getCurrentFrame());
            for (const auto& eventName : events) {
                EntityAnimationEvent event;
                event.entity = entt::handle{registry, e};
                event.event = eventName;
                em.triggerEvent(event);
            }
        }
    }
}

// TODO: do this in state machine instead
inline void playerAnimationSystemUpdate(
    entt::handle player,
    const PhysicsSystem& physicsSystem,
    float dt)
{
    namespace eu = entityutil;
    auto& mc = player.get<MovementComponent>();
    auto velocity = mc.effectiveVelocity;
    velocity.y = 0.f;
    const auto velMag = glm::length(velocity);

    bool playerRunning = velMag > 1.5f;

    if (!physicsSystem.isCharacterOnGround()) {
        if (mc.effectiveVelocity.y > 0.f) {
            eu::setAnimation(player, "Jump");
        } else {
            eu::setAnimation(player, "Fall");
        }
    } else {
        if (std::abs(velMag) <= 0.1f) {
            const auto& animator = player.get<SkeletonComponent>().skeletonAnimator;
            if (animator.getCurrentAnimationName() == "Run" ||
                animator.getCurrentAnimationName() == "Walk" ||
                animator.getCurrentAnimationName() == "Jump" ||
                animator.getCurrentAnimationName() == "Fall") {
                // eu::setAnimation(player, "Think");
                eu::setAnimation(player, "Idle");
            }
        } else {
            // ^^^
            // when sliding against the wall, "Idle" and "Walk" animations
            // change too quick - this prevents that (but we might not need it
            // when blending is added)
            if (playerRunning) {
                eu::setAnimation(player, "Run");
            } else if (velMag > 0.2f) {
                // HACK: sometimes the player can fall slightly when being spawned
                eu::setAnimation(player, "Walk");
            }
        }
    }
}
