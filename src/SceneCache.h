#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include <Graphics/Scene.h>

class BaseRenderer;

class SceneCache {
public:
    [[nodiscard]] const Scene& loadScene(BaseRenderer& renderer, const std::filesystem::path& path);

private:
    std::unordered_map<std::string, Scene> sceneCache;
};
