#pragma once

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MovementComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>
#include <edbr/Event/EventManager.h>

#include "Components.h"
#include "EntityUtil.h"
#include "Events.h"
#include "PhysicsSystem.h"

inline void skeletonAnimationSystemUpdate(entt::registry& registry, EventManager& em, float dt)
{
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

inline void blinkSystemUpdate(entt::registry& registry, float dt)
{
    for (const auto&& [e, bc, fc] : registry.view<BlinkComponent, FaceComponent>().each()) {
        if (!bc.isBlinking && !bc.faces.contains(fc.currentFace)) {
            // current face doesn't have blink animation
            bc.isBlinking = false;
            bc.timer = 0.f;
            bc.blinkFace.clear();
            bc.nonBlinkFace.clear();
            continue;
        }

        // check if face was changed externally
        if ((bc.isBlinking && bc.blinkFace != fc.currentFace) ||
            (!bc.isBlinking && bc.nonBlinkFace != fc.currentFace)) {
            bc.isBlinking = false;
            bc.timer = 0.f;
            bc.nonBlinkFace = fc.currentFace;
        }

        if (bc.isBlinking) {
            bc.timer += dt;
            if (bc.timer >= bc.blinkHold) {
                bc.isBlinking = false;
                bc.timer = 0.f;

                // restore non-blink face
                entityutil::setFace({registry, e}, bc.nonBlinkFace);
            }
        } else {
            bc.timer += dt;
            if (bc.timer >= bc.blinkPeriod) {
                bc.isBlinking = true;
                bc.timer = 0.f;

                // set blink-face
                bc.nonBlinkFace = fc.currentFace;
                bc.blinkFace = bc.faces.at(fc.currentFace);
                entityutil::setFace({registry, e}, bc.blinkFace);
            }
        }
    }
}
