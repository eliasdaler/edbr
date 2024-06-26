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
    SceneCache(
        GfxDevice& gfxDevice,
        MeshCache& meshCache,
        MaterialCache& materialCache,
        SkeletalAnimationCache& animationCache);

    const Scene& addScene(const std::string& scenePath, Scene scene);
    const Scene& getScene(const std::string& scenePath) const;

    [[nodiscard]] const Scene& loadOrGetScene(const std::filesystem::path& path);

private:
    std::unordered_map<std::string, Scene> sceneCache;

    GfxDevice& gfxDevice;
    MeshCache& meshCache;
    MaterialCache& materialCache;
    SkeletalAnimationCache& animationCache;
};
