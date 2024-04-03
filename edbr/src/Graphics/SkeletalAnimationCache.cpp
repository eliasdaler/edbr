#include <edbr/Graphics/SkeletalAnimationCache.h>

#include <edbr/Core/JsonFile.h>

void SkeletalAnimationCache::loadAnimationData(const std::filesystem::path& path)
{
    JsonFile file(path);
    if (!file.isGood()) {
        throw std::runtime_error(
            fmt::format("failed to load animation data from {}", path.string()));
    }
    const auto loader = file.getLoader();

    for (const auto& animsLoader : loader.getVector()) {
        std::string gltfPath;
        animsLoader.get("mesh_path", gltfPath);
        auto& animDatas = animationData[gltfPath];
        for (const auto& [animName, animLoader] :
             animsLoader.getLoader("animations").getKeyValueMap()) {
            AnimationData data;
            animLoader.get("looped", data.looped, true);
            animLoader.get("startFrame", data.startFrame, 0);
            if (animLoader.hasKey("events")) {
                for (const auto& el : animLoader.getLoader("events").getVector()) {
                    std::string name;
                    el.get("name", name);
                    int frame;
                    el.get("frame", frame);
                    data.events[frame].push_back(std::string(name));
                }
            }
            animDatas.emplace(animName, std::move(data));
        }
    }
}

void SkeletalAnimationCache::addAnimations(
    const std::filesystem::path& gltfPath,
    AnimationsMap anims)
{
    // append animation data
    if (auto it = animationData.find(gltfPath.string()); it != animationData.end()) {
        for (const auto& [animName, data] : it->second) {
            auto& animation = anims.at(animName);
            animation.looped = data.looped;
            animation.startFrame = data.startFrame;
            animation.events = data.events;
        }
    }

    animations.emplace(gltfPath.string(), std::move(anims));
}

const SkeletalAnimationCache::AnimationsMap& SkeletalAnimationCache::getAnimations(
    const std::filesystem::path& gltfPath) const
{
    return animations.at(gltfPath.string());
}
