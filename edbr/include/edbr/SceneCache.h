#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include <edbr/Graphics/Scene.h>

class SkeletalAnimationCache;
class GfxDevice;
class MeshCache;
class MaterialCache;

class SceneCache {
public:
    SceneCache(SkeletalAnimationCache& animationCache);

    [[nodiscard]] const Scene& loadScene(
        GfxDevice& gfxDevice,
        MeshCache& meshCache,
        MaterialCache& materialCache,
        const std::filesystem::path& path);

private:
    std::unordered_map<std::string, Scene> sceneCache;
    SkeletalAnimationCache& animationCache;
};
