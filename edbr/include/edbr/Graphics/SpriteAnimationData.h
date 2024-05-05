#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include <edbr/Graphics/SpriteAnimation.h>
#include <edbr/Graphics/SpriteSheet.h>

class SpriteAnimationData {
public:
    void load(const std::filesystem::path& path);

    const SpriteAnimation& getAnimation(const std::string& animName) const
    {
        return animations.at(animName);
    }

    const SpriteSheet& getSpriteSheet() const { return spriteSheet; }

private:
    std::unordered_map<std::string, SpriteAnimation> animations;
    SpriteSheet spriteSheet;
};
