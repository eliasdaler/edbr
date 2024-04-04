#pragma once

#include <functional>
#include <vector>

#include <entt/entity/fwd.hpp>

class EntityFactory;
class SceneCache;
struct Scene;
struct SceneNode;

namespace util
{

// Maps Blender scene node names to game prefab names, e.g.
// * "SomePrefab" -> "some_prefab"
// * "SomePrefab.2" -> "some_prefab"
// * "SomePrefab.2" -> "some_prefab" -> "static_geometry" (if "some_prefab" had custom mapping)
// Returns an empty string if no mapping is found
std::string getPrefabNameFromSceneNode(const EntityFactory& ef, const std::string& gltfNodeName);
} // end of namespace util

struct EntityCreator {
public:
    EntityCreator(
        entt::registry& registry,
        std::string defaultPrefabName,
        EntityFactory& entityFactory,
        SceneCache& sceneCache);

    entt::handle createFromPrefab(const std::string& prefabName);
    std::vector<entt::handle> createEntitiesFromScene(const Scene& scene);

    void setPostInitEntityFunc(std::function<void(entt::handle e)> f) { postInitEntityFunc = f; }

private:
    entt::handle createFromPrefab(
        const std::string& prefabName,
        const Scene* creationScene,
        const SceneNode* creationNode);
    std::string getPrefabName(const SceneNode& node) const;
    void processNode(entt::handle e, const Scene& scene, const SceneNode& rootNode);

    entt::registry& registry;
    std::string defaultPrefabName;
    EntityFactory& entityFactory;
    SceneCache& sceneCache;

    std::function<void(entt::handle e)> postInitEntityFunc;
};
