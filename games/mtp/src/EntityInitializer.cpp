#include "EntityInitializer.h"

#include "Components.h"
#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/Components/NameComponent.h>
#include <edbr/Graphics/GameRenderer.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/MeshCache.h>
#include <edbr/Graphics/SkeletalAnimationCache.h>
#include <edbr/SceneCache.h>

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>
#include <fmt/printf.h>

namespace
{
std::string extractNameFromSceneNodeName(const std::string& sceneNodeName)
{
    if (sceneNodeName.empty()) {
        return {};
    }
    const auto dotPos = sceneNodeName.find_first_of(".");
    if (dotPos == std::string::npos || dotPos == sceneNodeName.length() - 1) {
        fmt::println(
            "ERROR: cannot extract name {}: should have format <prefab>.<name>", sceneNodeName);
    }
    return sceneNodeName.substr(dotPos + 1, sceneNodeName.size());
}
}

EntityInitializer::EntityInitializer(
    SceneCache& sceneCache,
    GfxDevice& gfxDevice,
    MeshCache& meshCache,
    MaterialCache& materialCache,
    SkeletalAnimationCache& animationCache) :
    sceneCache(sceneCache),
    gfxDevice(gfxDevice),
    meshCache(meshCache),
    materialCache(materialCache),
    animationCache(animationCache)
{}

void EntityInitializer::initEntity(entt::handle e) const
{
    if (auto mcPtr = e.try_get<MeshComponent>(); mcPtr) {
        auto& mc = *mcPtr;

        const auto& scene = sceneCache.loadScene(gfxDevice, meshCache, materialCache, mc.meshPath);
        assert(!scene.nodes.empty());
        const auto& node = *scene.nodes[0];

        mc.meshes.push_back(node.meshIndex);
        mc.meshTransforms.push_back(glm::mat4{1.f});

        const auto& primitives = scene.meshes[node.meshIndex].primitives;
        mc.meshes = primitives;
        mc.meshTransforms.resize(primitives.size());

        // handle non-identity transform of the model
        auto& tc = e.get<TransformComponent>();
        tc.transform = node.transform;

        // merge child meshes into the parent
        // FIXME: this is the same logic as in createEntityFromNode
        // and probably needs to work recursively for all children
        for (const auto& childPtr : node.children) {
            if (!childPtr) {
                continue;
            }
            const auto& childNode = *childPtr;
            auto& mc = e.get_or_emplace<MeshComponent>();
            for (const auto& meshId : scene.meshes[childNode.meshIndex].primitives) {
                mc.meshes.push_back(meshId);
                mc.meshTransforms.push_back(childNode.transform);
            }
        }

        if (node.skinId != -1) { // add skeleton component if skin exists
            auto& sc = e.get_or_emplace<SkeletonComponent>();
            sc.skeleton = scene.skeletons[node.skinId];
            // FIXME: this is bad - we need to have some sort of cache
            // and not copy animations everywhere
            sc.animations = &animationCache.getAnimations(mc.meshPath);

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
    }

    // extract player spawn name from scene node name
    if (e.all_of<PlayerSpawnComponent>()) {
        const auto& sceneNodeName = e.get<MetaInfoComponent>().sceneNodeName;
        if (sceneNodeName.empty()) { // created manually
            return;
        }
        e.get_or_emplace<NameComponent>().name = extractNameFromSceneNodeName(sceneNodeName);
    }

    // extract camera name from scene node name
    if (e.all_of<CameraComponent>()) {
        const auto& sceneNodeName = e.get<MetaInfoComponent>().sceneNodeName;
        if (sceneNodeName.empty()) { // created manually
            return;
        }
        e.get_or_emplace<NameComponent>().name = extractNameFromSceneNodeName(sceneNodeName);
    }

    // extract camera name from scene node name
    if (e.all_of<TriggerComponent>()) {
        const auto& sceneNodeName = e.get<MetaInfoComponent>().sceneNodeName;
        if (sceneNodeName.empty()) { // created manually
            return;
        }
        e.get_or_emplace<NameComponent>().name = extractNameFromSceneNodeName(sceneNodeName);
    }
}
