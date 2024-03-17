#pragma once

#include <Graphics/IdTypes.h>

#include <Math/Rect.h>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

struct AllocatedImage;

class Sprite {
public:
    Sprite() = default;
    Sprite(const AllocatedImage& texture);

    void setTexture(const AllocatedImage& texture);
    // Set texture rect by pixel coordinates, e.g. sprite.setTextureRect({32, 48, 16, 16});
    void setTextureRect(const math::IntRect& textureRect);

    ImageId texture{NULL_IMAGE_ID};
    glm::vec2 textureSize{0.f};

    glm::vec2 uv0{0.f}; // top-left UV coordinate
    glm::vec2 uv1{1.f}; // bottom-right UV coordinate

    // the origin of transforms - e.g. if you want the sprite to rotate around
    // its center, you can set this to glm::vec2{0.5, 0.5}
    glm::vec2 pivot{0.f};

    glm::vec4 color{1.f}; // the color the sprite is multiplied by in the shader
};
