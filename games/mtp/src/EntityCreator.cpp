#include "EntityCreator.h"

#include <edbr/ECS/Components/MetaInfoComponent.h>

#include <edbr/ECS/EntityFactory.h>
#include <edbr/SceneCache.h>

#include "Components.h"
#include "EntityUtil.h"
#include "GameSceneLoader.h"

entt::handle EntityCreator::createFromPrefab(
    const std::string& prefabName,
    const Scene* creationScene,
    const SceneNode* creationNode)
{
    const std::string emptyString{};
    const auto& nodeName = creationNode ? creationNode->name : emptyString;
    auto e = entityFactory.createEntity(registry, prefabName, nodeName);

    // load from external (prefab) scene
    auto& mic = e.get<MetaInfoComponent>();
    if (!mic.sceneName.empty()) {
        auto& scene = sceneCache.getScene(mic.sceneName);
        assert(scene.nodes.size() == 1);
        // TODO: support multiple root nodes here?
        // Can find node with mic.sceneNodeName on the scene and use it as a
        // root, for example.
        processNode(e, scene, *scene.nodes[0]);
        mic.creationSceneName = mic.sceneName;
    }

    // this is the scene this prefab was created from - process its children too
    if (creationScene) {
        assert(creationNode);
        processNode(e, *creationScene, *creationNode);
        mic.creationSceneName = creationScene->path.string();
    }

    return {registry, e};
}

std::string EntityCreator::getPrefabName(const SceneNode& node) const
{
    if (node.lightId != -1) {
        return "light";
    }
    if (node.cameraId != -1) {
        return "camera";
    }

    const auto name = util::getPrefabNameFromSceneNode(entityFactory, node.name);
    if (!name.empty()) {
        return name;
    }
    return defaultPrefabName;
}

std::vector<entt::handle> EntityCreator::createEntitiesFromScene(const Scene& scene)
{
    assert(entityFactory.prefabExists("camera"));
    assert(entityFactory.prefabExists("light"));

    std::vector<entt::handle> createdEntities;
    for (const auto& rootNode : scene.nodes) {
        auto e = createFromPrefab(getPrefabName(*rootNode), &scene, rootNode.get());
        createdEntities.push_back(std::move(e));
    }
    return createdEntities;
}

void EntityCreator::processNode(entt::handle e, const Scene& scene, const SceneNode& rootNode)
{
    // copy transform
    auto& tc = e.get<TransformComponent>();
    tc.transform = rootNode.transform;

    // camera
    if (rootNode.cameraId != -1) {
        assert(e.get<MetaInfoComponent>().prefabName == "camera");
        // camera is flipped in glTF ("Z" is pointing backwards)
        // so we need to rotate 180 degrees around Y
        auto& tc = e.get<TransformComponent>();
        tc.transform.setHeading(
            glm::angleAxis(glm::radians(180.f), tc.transform.getLocalUp()) *
            tc.transform.getHeading());
    }

    // light
    if (rootNode.lightId != -1) {
        assert(e.get<MetaInfoComponent>().prefabName == "light");
        auto& lc = e.get_or_emplace<LightComponent>();
        lc.light = scene.lights[rootNode.lightId];
    }

    // copy meshes
    if (rootNode.meshIndex != -1) {
        auto& mc = e.get_or_emplace<MeshComponent>();
        if (mc.meshes.empty()) {
            for (const auto& id : scene.meshes[rootNode.meshIndex].primitives) {
                mc.meshes.push_back(id);
            }
        } else {
            // throw std::runtime_error(
            //    "TODO: handle replacing meshes when processing scene node");
            // TODO: what to do here?
            // Prefab can have a mesh, but would also have it in a level
            // for visualization yet we don't want to copy this mesh...
        }
    }

    // handle children
    for (const auto& cNodePtr : rootNode.children) {
        auto& cNode = *cNodePtr;
        const auto childNodePrefabName = getPrefabName(cNode);
        if (childNodePrefabName != defaultPrefabName) {
            auto child = createFromPrefab(getPrefabName(cNode), &scene, &cNode);
            entityutil::addChild(e, child);
            continue;
        }

        assert(cNode.children.empty()); // TODO: handle this too?
        // e.g. the child is a mesh, but has a light/prefab as a child...
        if (cNode.meshIndex != -1) {
            // merge child meshes into the parent node
            auto& mc = e.get_or_emplace<MeshComponent>();
            for (const auto& id : scene.meshes[cNode.meshIndex].primitives) {
                mc.meshes.push_back(id);
            }
        }
    }
}
