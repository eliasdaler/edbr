#pragma once

#include <string>

#include <entt/entity/fwd.hpp>

class EntityFactory;
class GfxDevice;
class MaterialCache;
class MeshCache;
class PhysicsSystem;
class SceneCache;
class SkeletalAnimationCache;
struct Scene;

namespace util
{
struct SceneLoadContext {
    GfxDevice& gfxDevice;
    MeshCache& meshCache;
    MaterialCache& materialCache;
    EntityFactory& entityFactory;
    entt::registry& registry;
    SceneCache& sceneCache;
    std::string defaultPrefabName;
    PhysicsSystem& physicsSystem;
    SkeletalAnimationCache& animationCache;
};

void createEntitiesFromScene(const SceneLoadContext& loadCtx, const Scene& scene);
void onPlaceEntityOnScene(const util::SceneLoadContext& loadCtx, entt::handle e);

} // end of namespace util
