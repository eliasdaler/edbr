#pragma once

#include <string>

#include <entt/entity/fwd.hpp>

class GameRenderer;
class EntityFactory;
class SceneCache;
struct Scene;
class PhysicsSystem;
class MaterialCache;
class SkeletalAnimationCache;

namespace util
{
struct SceneLoadContext {
    GameRenderer& renderer;
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
