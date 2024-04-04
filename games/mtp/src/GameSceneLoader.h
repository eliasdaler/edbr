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

// Maps Blender scene node names to game prefab names, e.g.
// * "SomePrefab" -> "some_prefab"
// * "SomePrefab.2" -> "some_prefab"
// * "SomePrefab.2" -> "some_prefab" -> "static_geometry" (if "some_prefab" had custom mapping)
// Returns an empty string if no mapping is found
std::string getPrefabNameFromSceneNode(const EntityFactory& ef, const std::string& gltfNodeName);

} // end of namespace util
