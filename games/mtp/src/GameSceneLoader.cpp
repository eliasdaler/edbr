#include "GameSceneLoader.h"

#include <edbr/Graphics/Scene.h>
#include <edbr/Util/StringUtil.h>

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/EntityFactory.h>

#include <edbr/Graphics/GameRenderer.h>

#include "Components.h"

namespace
{
// Maps Blender scene node names to game prefab names, e.g.
// * "SomePrefab" -> "some_prefab"
// * "SomePrefab.2" -> "some_prefab"
// * "SomePrefab.2" -> "some_prefab" -> "static_geometry" (if "some_prefab" had custom mapping)
// Returns an empty string if no mapping is found
std::string getPrefabNameFromSceneNode(const EntityFactory& ef, const SceneNode& node)
{
    const auto nodeName = util::fromCamelCaseToSnakeCase(node.name);
    const auto dotPos = nodeName.find_first_of(".");
    if (dotPos == std::string::npos) {
        // when node name == prefab name (e.g. node name == "TestPrefab" and
        // "test_prefab" exists)
        if (const auto& name = ef.getMappedPrefabName(nodeName); !name.empty()) {
            return name;
        }
    } else {
        // node name == "TestPrefab.001" and "test_prefab" exists
        const auto substr = nodeName.substr(0, dotPos);
        if (const auto& name = ef.getMappedPrefabName(substr); !name.empty()) {
            return name;
        }
    }
    return "";
}

struct EntityCreateInfo {
    entt::handle entity;
    bool createdFromPrefab{false};
};

EntityCreateInfo createEntityFromNode(
    const util::SceneLoadContext& loadCtx,
    const Scene& scene,
    const SceneNode& node,
    entt::handle parent)
{
    auto& registry = loadCtx.registry;
    const auto nodePrefabName = getPrefabNameFromSceneNode(loadCtx.entityFactory, node);
    const auto prefabName = !nodePrefabName.empty() ? nodePrefabName : loadCtx.defaultPrefabName;

    auto e = prefabName.empty() ?
                 loadCtx.entityFactory.createDefaultEntity(registry, node.name) :
                 loadCtx.entityFactory.createEntity(registry, prefabName, node.name);

    auto& hc = e.get<HierarchyComponent>();
    { // establish child-parent relationship (children are loaded later)
        hc.parent = parent;
        if (hc.hasParent()) {
            auto& parentHC = registry.get<HierarchyComponent>(parent);
            parentHC.children.push_back(e);
        }
    }

    { // transform
        auto& tc = e.get<TransformComponent>();
        // note: sometimes transform can already be non-identity due
        // to scaling of the model which was used when creating the prefab
        tc.transform = node.transform * tc.transform;
        if (!hc.hasParent()) {
            tc.worldTransform = tc.transform.asMatrix();
        } else {
            // TODO: this might potentially be done automatically by hierarchy system?
            const auto& parentTC = registry.get<TransformComponent>(parent);
            tc.worldTransform = parentTC.worldTransform * tc.transform.asMatrix();
        }
    }

    if (node.meshIndex != -1 && !e.any_of<MeshComponent, PlayerSpawnComponent>()) { // load mesh
                                                                                    // from scene
        const auto& primitives = scene.meshes[node.meshIndex].primitives;
        auto& mc = e.emplace<MeshComponent>();
        mc.meshes = primitives;
        mc.meshTransforms.resize(primitives.size());

        // skeleton
        if (node.skinId != -1) {
            auto& sc = e.emplace<SkeletonComponent>();
            sc.skeleton = scene.skeletons[node.skinId];
            // FIXME: this is bad - we need to have some sort of cache
            // and not copy animations everywhere
            sc.animations = scene.animations;

            sc.skinnedMeshes.reserve(mc.meshes.size());
            for (const auto meshId : mc.meshes) {
                sc.skinnedMeshes.push_back(loadCtx.renderer.createSkinnedMesh(meshId));
            }
        }
    }

    if (node.lightId != -1 && !e.all_of<LightComponent>()) {
        auto& lc = e.emplace<LightComponent>();
        lc.light = scene.lights[node.lightId];
        e.get<MetaInfoComponent>().prefabName = "light";
    }

    if (node.cameraId != -1 || e.all_of<CameraComponent>()) {
        e.get<MetaInfoComponent>().prefabName = "camera";

        // camera is flipped in glTF ("Z" is pointing backwards)
        // so we need to rotate 180 degrees around Y
        auto& tc = e.get<TransformComponent>();
        tc.transform.setHeading(
            glm::angleAxis(glm::radians(180.f), tc.transform.getLocalUp()) *
            tc.transform.getHeading());
    }

    bool createdFromPrefab = !nodePrefabName.empty();
    return {
        .entity = e,
        .createdFromPrefab = createdFromPrefab,
    };
}

void createEntitiesFromNode(
    const util::SceneLoadContext& loadCtx,
    const Scene& scene,
    const SceneNode& node,
    entt::handle parent)
{
    const auto loadInfo = createEntityFromNode(loadCtx, scene, node, parent);
    assert(loadInfo.entity.entity() != entt::null);
    auto e = loadInfo.entity;
    if (loadInfo.createdFromPrefab) {
        return;
    }

    auto& hc = e.get<HierarchyComponent>();
    for (const auto& childPtr : node.children) {
        if (!childPtr) {
            continue;
        }
        const auto& childNode = *childPtr;

        // if the child doesn't have children, we can merge its meshes
        // into the parent entity so that not too many entities are created
        // note: this is not supported for skinned meshes.
        bool canMergeChildrenIntoParent =
            (childNode.meshIndex != -1 && childNode.children.empty() && node.skinId == -1);

        if (canMergeChildrenIntoParent) {
            auto& mc = e.get_or_emplace<MeshComponent>();
            for (const auto& meshId : scene.meshes[childNode.meshIndex].primitives) {
                mc.meshes.push_back(meshId);
                mc.meshTransforms.push_back(childNode.transform);
            }
        } else {
            createEntitiesFromNode(loadCtx, scene, childNode, e);
        }
    }
}

} // end of anonymous namespace

namespace util
{

void createEntitiesFromScene(const SceneLoadContext& loadCtx, const Scene& scene)
{
    for (const auto& nodePtr : scene.nodes) {
        if (nodePtr) {
            ::createEntitiesFromNode(loadCtx, scene, *nodePtr, {});
        }
    }
}

} // end of namespace util
