#include <edbr/SceneCache.h>

#include <fmt/printf.h>

#include <edbr/Graphics/SkeletalAnimationCache.h>
#include <edbr/Util/GltfLoader.h>

SceneCache::SceneCache(
    GfxDevice& gfxDevice,
    MeshCache& meshCache,
    MaterialCache& materialCache,
    SkeletalAnimationCache& animationCache) :
    gfxDevice(gfxDevice),
    meshCache(meshCache),
    materialCache(materialCache),
    animationCache(animationCache)
{}

const Scene& SceneCache::addScene(const std::string& scenePath, Scene scene)
{
    scene.path = scenePath;
    auto [it, inserted] = sceneCache.emplace(scenePath, std::move(scene));
    assert(inserted && "Scene was added before");
    return it->second;
}

const Scene& SceneCache::getScene(const std::string& scenePath) const
{
    auto it = sceneCache.find(scenePath);
    if (it == sceneCache.end()) {
        throw std::runtime_error(fmt::format("Scene '{}' was not loaded", scenePath));
    }
    return it->second;
}

const Scene& SceneCache::loadOrGetScene(const std::filesystem::path& path)
{
    const auto it = sceneCache.find(path.string());
    if (it != sceneCache.end()) {
        // fmt::print("Using cached gltf scene '{}'\n", path.string());
        return it->second;
    }

    fmt::print("Loading gltf scene '{}'\n", path.string());

    auto scene = util::loadGltfFile(gfxDevice, meshCache, materialCache, path);
    if (!scene.animations.empty()) {
        // NOTE: we don't move here so that the returned/cached scene still
        // has animations inspectable in it
        animationCache.addAnimations(path, scene.animations);
    }
    const auto [it2, inserted] = sceneCache.emplace(path.string(), std::move(scene));
    assert(inserted);
    return it2->second;
}
