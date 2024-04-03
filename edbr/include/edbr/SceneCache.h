#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include <edbr/Graphics/Scene.h>

class BaseRenderer;
class SkeletalAnimationCache;

class SceneCache {
public:
    SceneCache(SkeletalAnimationCache& animationCache);

    [[nodiscard]] const Scene& loadScene(BaseRenderer& renderer, const std::filesystem::path& path);

private:
    std::unordered_map<std::string, Scene> sceneCache;
    SkeletalAnimationCache& animationCache;
};
