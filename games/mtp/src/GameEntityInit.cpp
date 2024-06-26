#include "Game.h"

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/Components/NPCComponent.h>
#include <edbr/ECS/Components/NameComponent.h>
#include <edbr/ECS/Components/SceneComponent.h>

#include "EntityUtil.h"

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
        const auto& sceneNodeName = e.get<SceneComponent>().sceneNodeName;
        if (sceneNodeName.empty()) { // created manually
            return;
        }
        e.get_or_emplace<NameComponent>().name = extractNameFromSceneNodeName(sceneNodeName);
    }

    if (e.all_of<FaceComponent>()) {
        auto& fc = e.get<FaceComponent>();

        // find face mesh
        auto& mc = e.get<MeshComponent>();
        fc.faceMeshIndex = [&mc, this]() {
            for (std::size_t i = 0; i < mc.meshes.size(); ++i) {
                const auto& material = materialCache.getMaterial(mc.meshMaterials[i]);
                if (material.name == "Face") {
                    return i;
                }
            }
            return std::string::npos;
        }();
        if (fc.faceMeshIndex == std::string::npos) {
            throw std::runtime_error("FaceComponent: mesh with material 'Face' was not found");
        }

        // create face materials
        fc.faces.reserve(fc.facesFilenames.size());
        const auto& prefabName = e.get<MetaInfoComponent>().prefabName;
        for (const auto& [faceName, filename] : fc.facesFilenames) {
            FaceComponent::FaceData fd{};
            const auto texturePath = fc.facesTexturesDir / (filename + ".png");

            // copy
            auto faceMaterial = materialCache.getMaterial(mc.meshMaterials[fc.faceMeshIndex]);
            faceMaterial.name = fmt::format("{}.face.{}", prefabName, faceName);
            // change diffuse texture
            faceMaterial.diffuseTexture = gfxDevice.loadImageFromFile(
                texturePath, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, true);
            fd.materialId = materialCache.addMaterial(gfxDevice, faceMaterial);

            fc.faces.emplace(faceName, std::move(fd));
        }

        entityutil::setFace(e, fc.defaultFace);
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

    const auto& scc = e.get<SceneComponent>();
    const auto& scene = sceneCache.loadOrGetScene(scc.sceneName);

    sc.skeleton = scene.skeletons[sc.skinId];
    sc.animations = &animationCache.getAnimations(scc.sceneName);

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
