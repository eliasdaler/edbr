#pragma once

#include <string>

#include <entt/entity/fwd.hpp>

class GameRenderer;
class EntityFactory;
class SceneCache;
struct Scene;
class Game;
class PhysicsSystem;

namespace util
{
struct SceneLoadContext {
    GameRenderer& renderer;
    EntityFactory& entityFactory;
    entt::registry& registry;
    SceneCache& sceneCache;
    std::string defaultPrefabName;
    PhysicsSystem& physicsSystem;
};

void createEntitiesFromScene(const SceneLoadContext& loadCtx, const Scene& scene);
void onPlaceEntityOnScene(const util::SceneLoadContext& loadCtx, entt::handle e);

} // end of namespace util
