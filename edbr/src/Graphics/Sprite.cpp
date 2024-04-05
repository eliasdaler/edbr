#include <edbr/Graphics/Sprite.h>

#include <edbr/Graphics/Vulkan/GPUImage.h>

#include <cassert>

Sprite::Sprite(const GPUImage& texture)
{
    setTexture(texture);
}

void Sprite::setTexture(const GPUImage& texture)
{
    this->texture = static_cast<ImageId>(texture.getBindlessId());
    textureSize = glm::vec2{texture.getExtent2D().width, texture.getExtent2D().height};
}

// Set texture rect by pixel coordinates, e.g. sprite.setTextureRect({32, 48, 16, 16});
void Sprite::setTextureRect(const math::IntRect& textureRect)
{
    assert(texture != NULL_IMAGE_ID && "texture was not set");
    uv0 = static_cast<glm::vec2>(textureRect.getTopLeftCorner()) / textureSize;
    uv1 = static_cast<glm::vec2>(textureRect.getBottomRightCorner()) / textureSize;
}

void Sprite::setPivotPixel(const glm::ivec2& pixel)
{
    assert(texture != NULL_IMAGE_ID);
    pivot = static_cast<glm::vec2>(pixel) / textureSize;
}
