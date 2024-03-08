#pragma once

#include <filesystem>
#include <unordered_map>

#include <Graphics/IdTypes.h>

struct Scene;

class BaseRenderer;

namespace util
{
struct LoadContext {
    BaseRenderer& renderer;
};

class SceneLoader {
public:
    void loadScene(const LoadContext& context, Scene& scene, const std::filesystem::path& path);

private:
    // gltf material id -> material cache id
    std::unordered_map<std::size_t, MaterialId> materialMapping;

    // gltf node id -> JointId
    // for now only one skeleton per scene is supported
    std::unordered_map<int, JointId> gltfNodeIdxToJointId;
};

}
