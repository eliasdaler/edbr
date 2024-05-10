#include "Game.h"

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/Components/NPCComponent.h>
#include <edbr/ECS/Components/NameComponent.h>

namespace
{
std::string extractNameFromSceneNodeName(const std::string& sceneNodeName)
{
    if (sceneNodeName.empty()) {
        return {};
    }
    const auto dotPos = sceneNodeName.find_last_of(".");
    if (dotPos == std::string::npos || dotPos == sceneNodeName.length() - 1) {
        fmt::println(
            "cannot extract name from node '{}': should have format <prefab>.<name>",
            sceneNodeName);
    }
    return sceneNodeName.substr(dotPos + 1, sceneNodeName.size());
}
}

void Game::entityPostInit(entt::handle e)
{
    // handle skeleton and animation
    if (auto scPtr = e.try_get<SkeletonComponent>(); scPtr) {
        initEntityAnimation(e);
    }

    // physics
    if (auto pcPtr = e.try_get<PhysicsComponent>(); pcPtr) {
        physicsSystem->addEntity(e, sceneCache);
    }

    // For NPCs, add InteractComponent with "Talk" type if not added already
    if (e.all_of<NPCComponent>() && !e.all_of<InteractComponent>()) {
        auto& ic = e.emplace<InteractComponent>();
        ic.type = InteractComponent::Type::Talk;
    }

    // extract name from scene node
    if (e.any_of<PlayerSpawnComponent, CameraComponent, TriggerComponent, NPCComponent>()) {
        const auto& sceneNodeName = e.get<MetaInfoComponent>().sceneNodeName;
        if (sceneNodeName.empty()) { // created manually
            return;
        }
        e.get_or_emplace<NameComponent>().name = extractNameFromSceneNodeName(sceneNodeName);
    }

    // init children
    for (const auto& c : e.get<HierarchyComponent>().children) {
        entityPostInit(c);
    }
}

void Game::initEntityAnimation(entt::handle e)
{
    auto& sc = e.get<SkeletonComponent>();
    assert(sc.skinId != -1);

    auto& mic = e.get<MetaInfoComponent>();
    const auto& scene = sceneCache.loadOrGetScene(mic.sceneName);

    sc.skeleton = scene.skeletons[sc.skinId];
    sc.animations = &animationCache.getAnimations(mic.sceneName);

    auto& mc = e.get<MeshComponent>();
    sc.skinnedMeshes.reserve(mc.meshes.size());
    for (const auto meshId : mc.meshes) {
        const auto& mesh = meshCache.getMesh(meshId);
        SkinnedMesh sm;
        sm.skinnedVertexBuffer = gfxDevice.createBuffer(
            mesh.numVertices * sizeof(CPUMesh::Vertex),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        sc.skinnedMeshes.push_back(sm);
    }

    if (sc.animations->contains("Idle")) {
        sc.skeletonAnimator.setAnimation(sc.skeleton, sc.animations->at("Idle"));
    }
}
