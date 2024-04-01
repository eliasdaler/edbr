#pragma once

#include <entt/entity/fwd.hpp>

class SceneCache;
class GameRenderer;
class PhysicsSystem;

class EntityInitializer {
public:
    EntityInitializer(SceneCache& sceneCache, GameRenderer& renderer);
    void setPhysicsSystem(PhysicsSystem& ps) { physicsSystem = &ps; }
    void initEntity(entt::handle e) const;

private:
    SceneCache& sceneCache;
    GameRenderer& renderer;
    PhysicsSystem* physicsSystem{nullptr};
};
