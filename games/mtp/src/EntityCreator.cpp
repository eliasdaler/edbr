#include "EntityCreator.h"

#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/Components/SceneComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>

#include <edbr/ECS/EntityFactory.h>
#include <edbr/SceneCache.h>
#include <edbr/Util/StringUtil.h>

#include "Components.h"
#include "EntityUtil.h"

namespace util
{
std::string getPrefabNameFromSceneNode(
    const EntityFactory& ef,
    const SceneNode& node,
    const std::string& defaultPrefabName)
{
    if (node.lightId != -1) {
        return "light";
    }
    if (node.cameraId != -1) {
        return "camera";
    }

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
    return defaultPrefabName;
}

void loadPhysicsComponent(
    PhysicsComponent& pc,
    const Scene& scene,
    const std::vector<MeshId> meshes,
    const std::string& nodeName)
{
    const auto aabb = edbr::calculateBoundingBoxLocal(scene, meshes);
    const auto aabbSize = aabb.calculateSize();
    if (util::stringContains(nodeName, ".Box")) {
        pc.bodyType = PhysicsComponent::BodyType::AABB;
        pc.bodyParams = PhysicsComponent::AABBParams{
            .min = aabb.min,
            .max = aabb.max,
        };
    }
    if (util::stringContains(nodeName, ".Sphere")) {
        pc.bodyType = PhysicsComponent::BodyType::Sphere;
        float maxExtent = std::max(aabbSize.x, aabbSize.y);
        maxExtent = std::max(maxExtent, aabbSize.z);

        pc.bodyParams = PhysicsComponent::SphereParams{
            .radius = maxExtent / 2.f,
        };
    }
    if (util::stringContains(nodeName, ".Cylinder")) {
        pc.bodyType = PhysicsComponent::BodyType::Cylinder;
        pc.bodyParams = PhysicsComponent::CylinderParams{
            .radius = std::max(aabbSize.x, aabbSize.z) / 2.f,
            .halfHeight = aabbSize.y / 2.f,
        };
    }
    if (util::stringContains(nodeName, ".Convex")) {
        pc.bodyType = PhysicsComponent::BodyType::ConvexHull;
    }
}

} // end of namespace util

EntityCreator::EntityCreator(
    entt::registry& registry,
    std::string defaultPrefabName,
    EntityFactory& entityFactory,
    SceneCache& sceneCache) :
    registry(registry),
    defaultPrefabName(defaultPrefabName),
    entityFactory(entityFactory),
    sceneCache(sceneCache)
{}

entt::handle EntityCreator::createFromPrefab(const std::string& prefabName, bool callPostInitFunc)
{
    assert(postInitEntityFunc);

    auto e = entityFactory.createEntity(registry, prefabName);

    // load from external (prefab) scene
    auto& sc = e.get<SceneComponent>();
    if (!sc.sceneName.empty()) {
        // load or get scene
        auto& scene = sceneCache.loadOrGetScene(sc.sceneName);
        assert(scene.nodes.size() == 1);
        // TODO: support multiple root nodes here?
        // Can find node with mic.sceneNodeName on the scene and use it as a
        // root, for example.
        auto& rootNode = *scene.nodes[0];
        processNode(e, scene, rootNode);
        sc.creationSceneName = sc.sceneName;
        sc.sceneNodeName = rootNode.name;
    }

    if (callPostInitFunc) {
        postInitEntityFunc(e);
    }

    return e;
}

std::vector<entt::handle> EntityCreator::createEntitiesFromScene(const Scene& scene)
{
    assert(postInitEntityFunc);
    assert(entityFactory.prefabExists("camera"));
    assert(entityFactory.prefabExists("light"));

    std::vector<entt::handle> createdEntities;
    for (const auto& rootNode : scene.nodes) {
        const auto prefabName =
            util::getPrefabNameFromSceneNode(entityFactory, *rootNode, defaultPrefabName);
        auto e = createFromNode(prefabName, scene, *rootNode);
        createdEntities.push_back(std::move(e));
    }
    for (const auto& e : createdEntities) {
        postInitEntityFunc(e);
    }
    return createdEntities;
}

entt::handle EntityCreator::createFromNode(
    const std::string& prefabName,
    const Scene& creationScene,
    const SceneNode& creationNode)
{
    auto e = createFromPrefab(prefabName, false);

    auto& sc = e.get<SceneComponent>();
    // this is the scene this prefab was created from - process its children too
    if (!sc.sceneName.empty() && creationNode.children.size() == 1 &&
        creationNode.children[0]->meshIndex != -1) {
        e.get<TransformComponent>().transform = creationNode.transform;
        // Only copy transform - this is a hack for Blender's "instanced" prefabs
        // there's sadly no way to detect them easily. Basically, we have
        // a node which has the only child - our prefab mesh
    } else {
        processNode(e, creationScene, creationNode);
        sc.creationSceneName = creationScene.path.string();
        sc.sceneNodeName = creationNode.name;
    }

    return e;
}

namespace
{
void appendMesh(MeshComponent& mc, const SceneMesh& mesh, const glm::mat4& transform)
{
    for (std::size_t i = 0; i < mesh.primitives.size(); ++i) {
        mc.meshes.push_back(mesh.primitives[i]);
        mc.meshMaterials.push_back(mesh.primitiveMaterials[i]);
        mc.meshTransforms.push_back(transform);
    }
}
}

void EntityCreator::processNode(entt::handle e, const Scene& scene, const SceneNode& rootNode)
{
    static const glm::mat4 I{1.f};

    // copy transform
    auto& tc = e.get<TransformComponent>();

    const auto prevTransform = tc.transform;
    tc.transform = rootNode.transform;
    if (!prevTransform.isIdentity()) {
        // sometimes the root node of prefab scene can have a non-identity transform
        // this is generally bad, but we can handle it (watch out, though)
        tc.transform = Transform(tc.transform.asMatrix() * prevTransform.asMatrix());
    }

    if (rootNode.skinId != -1) {
        auto& sc = e.get_or_emplace<SkeletonComponent>();
        assert(sc.skinId == -1);
        sc.skinId = rootNode.skinId;
    }

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
            const auto& sceneMesh = scene.meshes[rootNode.meshIndex];
            appendMesh(mc, sceneMesh, I);
        } else {
            // TODO: what to do here?
            // Prefab can have a mesh, but would also have it in a level
            // for visualization yet we don't want to copy this mesh...

            // throw std::runtime_error(
            //    "TODO: handle replacing meshes when processing scene node");
        }
    }

    // hack for triggers and colliders - determine shape base on node name
    if (e.any_of<TriggerComponent, ColliderComponent>()) {
        util::loadPhysicsComponent(
            e.get<PhysicsComponent>(), scene, e.get<MeshComponent>().meshes, rootNode.name);
    }

    // handle children
    for (const auto& cNodePtr : rootNode.children) {
        auto& cNode = *cNodePtr;
        const auto childNodePrefabName =
            util::getPrefabNameFromSceneNode(entityFactory, cNode, defaultPrefabName);

        if (childNodePrefabName == "collision") {
            assert(cNode.meshIndex != -1);
            auto& pc = e.get_or_emplace<PhysicsComponent>();
            util::loadPhysicsComponent(
                pc, scene, scene.meshes[cNode.meshIndex].primitives, cNode.name);
            continue;
        }

        if (childNodePrefabName != defaultPrefabName) {
            auto child = createFromNode(childNodePrefabName, scene, cNode);
            entityutil::addChild(e, child);
            continue;
        }

        assert(cNode.children.empty()); // TODO: handle this too?
        // e.g. the child is a mesh, but has a light/prefab as a child...
        if (cNode.meshIndex != -1) {
            // merge child meshes into the parent node
            auto& mc = e.get_or_emplace<MeshComponent>();
            const auto& childMesh = scene.meshes[cNode.meshIndex];
            appendMesh(mc, childMesh, cNode.transform.asMatrix());
        }
    }
}
