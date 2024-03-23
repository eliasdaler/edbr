#pragma once

#include <entt/entity/fwd.hpp>

class SceneCache;
class GameRenderer;

class EntityInitializer {
public:
    EntityInitializer(SceneCache& sceneCache, GameRenderer& renderer);
    void initEntity(entt::handle e) const;

private:
    SceneCache& sceneCache;
    GameRenderer& renderer;
};
