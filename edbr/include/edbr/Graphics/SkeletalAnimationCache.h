#pragma once

#include <filesystem>
#include <map>
#include <unordered_map>

#include <edbr/Graphics/SkeletalAnimation.h>

class SkeletalAnimationCache {
public:
    void loadAnimationData(const std::filesystem::path& path);

    using AnimationsMap = std::unordered_map<std::string, SkeletalAnimation>;
    void addAnimations(const std::filesystem::path& gltfPath, AnimationsMap anims);
    const AnimationsMap& getAnimations(const std::filesystem::path& gltfPath) const;

private:
    // gltf path -> animations
    std::unordered_map<std::string, AnimationsMap> animations;

    struct AnimationData {
        bool looped{true};
        std::map<int, std::vector<std::string>> events;
        int startFrame{0};
    };

    // gltf path -> animation name -> data
    std::unordered_map<std::string, std::unordered_map<std::string, AnimationData>> animationData;
};
