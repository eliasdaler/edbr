#pragma once

#include <glm/vec2.hpp>

#include <edbr/Graphics/Color.h>
#include <edbr/Graphics/IdTypes.h>

#include <edbr/Math/Rect.h>

struct GPUImage;

class Sprite {
public:
    Sprite() = default;
    Sprite(const GPUImage& texture);

    void setTexture(const GPUImage& texture);
    // Set texture rect by pixel coordinates, e.g. sprite.setTextureRect({32, 48, 16, 16});
    void setTextureRect(const math::IntRect& textureRect);

    ImageId texture{NULL_IMAGE_ID};
    glm::vec2 textureSize{0.f};

    glm::vec2 uv0{0.f}; // top-left UV coordinate
    glm::vec2 uv1{1.f}; // bottom-right UV coordinate

    // the origin of transforms - e.g. if you want the sprite to rotate around
    // its center, you can set this to glm::vec2{0.5, 0.5}
    glm::vec2 pivot{0.f};

    LinearColor color{1.f}; // the color the sprite is multiplied by in the shader
};
