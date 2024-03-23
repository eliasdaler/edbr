#include <edbr/SceneCache.h>

#include <fmt/printf.h>

#include <edbr/Util/GltfLoader.h>

const Scene& SceneCache::loadScene(BaseRenderer& renderer, const std::filesystem::path& path)
{
    const auto it = sceneCache.find(path.string());
    if (it != sceneCache.end()) {
        // fmt::print("Using cached gltf scene '{}'\n", path.string());
        return it->second;
    }

    fmt::print("Loading gltf scene '{}'\n", path.string());

    auto scene = util::loadGltfFile(renderer, path);
    const auto [it2, inserted] = sceneCache.emplace(path.string(), std::move(scene));
    assert(inserted);
    return it2->second;
}
