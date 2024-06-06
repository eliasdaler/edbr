#pragma once

#include <filesystem>

#include <edbr/Graphics/Sprite.h>

struct SpriteComponent {
    Sprite sprite;
    int z{0};
    std::filesystem::path spritePath;
};
