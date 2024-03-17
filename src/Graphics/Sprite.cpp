#include "Sprite.h"

#include <cassert>

#include <Graphics/Vulkan/Types.h>

Sprite::Sprite(const AllocatedImage& texture)
{
    setTexture(texture);
}

void Sprite::setTexture(const AllocatedImage& texture)
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
