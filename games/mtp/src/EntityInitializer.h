#pragma once

#include <entt/entity/fwd.hpp>

class SceneCache;
class GfxDevice;
class PhysicsSystem;
class MeshCache;
class MaterialCache;
class SkeletalAnimationCache;

class EntityInitializer {
public:
    EntityInitializer(
        SceneCache& sceneCache,
        GfxDevice& gfxDevice,
        MeshCache& meshCache,
        MaterialCache& materialCache,
        SkeletalAnimationCache& animationCache);
    void initEntity(entt::handle e) const;

private:
    SceneCache& sceneCache;
    GfxDevice& gfxDevice;
    MeshCache& meshCache;
    MaterialCache& materialCache;
    SkeletalAnimationCache& animationCache;
};
