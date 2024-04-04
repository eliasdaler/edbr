#pragma once

#include <vector>

#include <entt/entity/fwd.hpp>

class EntityFactory;
class SceneCache;
struct Scene;
struct SceneNode;

struct EntityCreator {
    entt::registry& registry;
    std::string defaultPrefabName;
    EntityFactory& entityFactory;
    SceneCache& sceneCache;

    entt::handle createFromPrefab(
        const std::string& prefabName,
        const Scene* creationScene = nullptr,
        const SceneNode* creationNode = nullptr);
    std::string getPrefabName(const SceneNode& node) const;
    std::vector<entt::handle> createEntitiesFromScene(const Scene& scene);
    void processNode(entt::handle e, const Scene& scene, const SceneNode& rootNode);
};
