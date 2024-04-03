#pragma once

#include <entt/entity/fwd.hpp>

class SceneCache;
class GameRenderer;
class PhysicsSystem;
struct MaterialCache;
class SkeletalAnimationCache;

class EntityInitializer {
public:
    EntityInitializer(
        SceneCache& sceneCache,
        GameRenderer& renderer,
        MaterialCache& materialCache,
        SkeletalAnimationCache& animationCache);
    void initEntity(entt::handle e) const;

private:
    SceneCache& sceneCache;
    GameRenderer& renderer;
    MaterialCache& materialCache;
    SkeletalAnimationCache& animationCache;
};
