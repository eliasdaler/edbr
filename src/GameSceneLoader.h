#pragma once

#include <entt/entt.hpp>

class GameRenderer;
class EntityFactory;
class SceneCache;
struct Scene;
class Game;

namespace util
{
struct SceneLoadContext {
    GameRenderer& renderer;
    EntityFactory& entityFactory;
    entt::registry& registry;
    SceneCache& sceneCache;
    std::string defaultPrefabName;
};

void createEntitiesFromScene(const SceneLoadContext& loadCtx, const Scene& scene);

} // end of namespace util
