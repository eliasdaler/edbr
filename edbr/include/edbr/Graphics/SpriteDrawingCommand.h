#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include <edbr/Graphics/Color.h>

// Keep in sync with sprite_pcs.glsl
struct SpriteDrawCommand {
    glm::mat4 transform;
    glm::vec2 uv0;
    glm::vec2 uv1;
    LinearColor color;
    std::uint32_t textureId;
    std::uint32_t shaderId;
    glm::vec2 padding;
};
