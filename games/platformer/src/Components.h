#pragma once

#include <edbr/Graphics/Sprite.h>
#include <edbr/Text/LocalizedStringTag.h>

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

struct NPCComponent {
    LocalizedStringTag name;
    LocalizedStringTag defaultText;
};

struct PlayerComponent {};
