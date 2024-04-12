#include <edbr/UI/ImageElement.h>

#include <glm/common.hpp>

namespace ui
{

ImageElement::ImageElement(const GPUImage& image, math::IntRect textureRect)
{
    sprite.setTexture(image);
    if (textureRect != math::IntRect{}) {
        sprite.setTextureRect(textureRect);
    }
}

void ImageElement::calculateOwnSize()
{
    absoluteSize = (sprite.uv1 - sprite.uv0) * sprite.textureSize;
    if (pixelPerfect) {
        absoluteSize = glm::round(absoluteSize);
    }
}

} // end of namespace ui
