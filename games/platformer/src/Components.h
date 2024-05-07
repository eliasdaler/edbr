#pragma once

#include <filesystem>

#include <edbr/Graphics/Sprite.h>
#include <edbr/Graphics/SpriteAnimationData.h>
#include <edbr/Graphics/SpriteAnimator.h>

struct TagComponent {
    std::string tag;
};

struct SpriteComponent {
    Sprite sprite;
    int z{0};
    std::filesystem::path spritePath;
};

struct CollisionComponent {
    glm::vec2 size;
    glm::vec2 origin;
};

struct CharacterControllerComponent {
public:
    float gravity{1.f};

    bool wasOnGround{false};
    bool isOnGround{false};
    bool wantJump{false};
};

struct SpriteAnimationComponent {
    SpriteAnimator animator;

    std::string defaultAnimationName{"idle"};
    SpriteAnimationData* animationsData{nullptr};
    std::string animationsDataTag;
};

struct SpawnerComponent {};

struct TeleportComponent {
    std::string levelTag;
    std::string spawnTag;
};

struct InteractComponent {
    enum class Type {
        None,
        Examine,
        Talk,
        GoInside,
    };
    Type type{Type::None};
};

struct PlayerComponent {};
