#pragma once

#include <filesystem>

#include <edbr/Graphics/Sprite.h>
#include <edbr/Graphics/SpriteAnimationData.h>
#include <edbr/Graphics/SpriteAnimator.h>

struct SpriteComponent {
    Sprite sprite;
    int z{0};
    std::filesystem::path spritePath;
};

struct CollisionComponent {
    glm::vec2 size;
    glm::vec2 origin;
};

struct SpriteAnimationComponent {
    SpriteAnimator animator;

    std::string defaultAnimationName{"idle"};
    SpriteAnimationData* animationsData{nullptr};
    std::string animationsDataTag;
};

struct PlayerComponent {};
