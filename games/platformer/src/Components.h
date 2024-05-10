#pragma once

#include <edbr/Graphics/Sprite.h>
#include <edbr/Text/LocalizedStringTag.h>

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

struct PlayerComponent {};
