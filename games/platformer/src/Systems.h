#pragma once

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include <glm/gtx/norm.hpp> // length2

#include <edbr/ECS/Components/MovementComponent.h>
#include <edbr/ECS/Components/SpriteAnimationComponent.h>
#include <edbr/ECS/Components/SpriteComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>
#include <edbr/TileMap/TileMap.h>

#include "Components.h"
#include "EntityUtil.h"

namespace
{
bool isSolidTile(const TileMap& tileMap, const TileMap::TileIndex& tileIndex)
{
    return tileMap.getTile(TileMap::CollisionLayerName, tileIndex).id == 0;
}

bool isOnGround(const entt::const_handle& e, const TileMap& tileMap)
{
    const auto eBB = entityutil::getAABB(e);
    const auto cp1 = eBB.getBottomLeftCorner() + glm::vec2{0.5f, 0.5f};
    const auto cp2 = eBB.getBottomLeftCorner() + glm::vec2{eBB.width / 2.f, 0.5f};
    const auto cp3 = eBB.getBottomRightCorner() + glm::vec2{-0.5f, 0.5f};

    bool onGround = false;

    for (const auto& p : {cp1, cp2, cp3}) {
        const auto ti = TileMap::GetTileIndex(p);
        if (isSolidTile(tileMap, ti)) {
            onGround = true;
        }
    }
    return onGround;
}
}

// set direction based on velocity
inline void directionSystemUpdate(entt::registry& registry, float dt)
{
    for (auto&& [e, tc, mc] : registry.view<TransformComponent, MovementComponent>().each()) {
        if (mc.effectiveVelocity.x != 0.f) {
            if (mc.kinematicVelocity.x > 0.f) {
                entityutil::setHeading2D({registry, e}, {1.f, 0.f});
            } else {
                entityutil::setHeading2D({registry, e}, {-1.f, 0.f});
            }
        }
    }
}

// TODO: do this in state machine
inline void playerAnimationSystemUpdate(entt::registry& registry, float dt, const TileMap& tileMap)
{
    auto player = entityutil::getPlayerEntity(registry);
    if (!isOnGround(player, tileMap)) {
        entityutil::setSpriteAnimation(player, "fall");
    } else {
        auto& mc = player.get<MovementComponent>();
        if (mc.effectiveVelocity.x != 0.f || mc.effectiveVelocity.y != 0.f) {
            entityutil::setSpriteAnimation(player, "run");
        } else {
            entityutil::setSpriteAnimation(player, "idle");
        }
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

inline void tileCollisionSystemUpdate(entt::registry& registry, float dt, const TileMap& tileMap)
{
    static constexpr auto MAX_ITERATIONS = 3;
    for (const auto&& [e, cc, mc] : registry.view<CollisionComponent, MovementComponent>().each()) {
        for (int i = 0; i < MAX_ITERATIONS; ++i) {
            const auto& eBB = entityutil::getAABB({registry, e});

            glm::vec2 minMtv{};
            bool minMTVSet = false;
            for (const auto& ti : tileMap.getTileIndicesInRect(eBB)) {
                if (!isSolidTile(tileMap, ti)) {
                    continue;
                }

                const auto tileBB = TileMap::GetTileAABB(ti);
                auto mtv = math::getIntersectionDepth(eBB, tileBB);

                if (std::abs(mtv.x) > std::abs(mtv.y)) {
                    mtv.x = 0.f; // will move vertically
                } else {
                    mtv.y = 0.f; // will move horizontally
                }

                if (!minMTVSet || glm::length2(mtv) < glm::length2(minMtv)) {
                    minMtv = mtv;
                    minMTVSet = true;
                }
            }

            if (minMTVSet) {
                // resolve collision
                auto pos = entityutil::getWorldPosition2D({registry, e});
                pos -= minMtv;
                entityutil::setWorldPosition2D({registry, e}, pos);
            }
        }
    }
}

inline void characterControlSystemUpdate(entt::registry& registry, float dt, const TileMap& tileMap)
{
    // feels good
    float TimeToReachApex = 0.45f;
    float MaxJumpHeight = 52.f;

    float Gravity = 2 * MaxJumpHeight / (TimeToReachApex * TimeToReachApex);
    float StartJumpVelocity = 2 * MaxJumpHeight / TimeToReachApex;

    float MaxFallSpeedY = 180.f;
    float MinMaxJumpSpeedX = 80.f;

    for (const auto&& [e, mc, cc] :
         registry.view<MovementComponent, CharacterControllerComponent>().each()) {
        cc.wasOnGround = cc.isOnGround;
        cc.isOnGround = isOnGround({registry, e}, tileMap);

        if (cc.isOnGround) {
            mc.kinematicVelocity.y = 0;
        }

        if (cc.wantJump && cc.wasOnGround && cc.isOnGround) {
            // ^ 1 frame delay for jumping
            cc.isOnGround = false;
            mc.kinematicVelocity.y = -StartJumpVelocity;
        }
        cc.wantJump = false;

        if (!cc.isOnGround) {
            mc.kinematicVelocity.y += cc.gravity * Gravity * dt;
            if (mc.kinematicVelocity.y > MaxFallSpeedY) {
                mc.kinematicVelocity.y = MaxFallSpeedY;
            }
        }
    }
}
