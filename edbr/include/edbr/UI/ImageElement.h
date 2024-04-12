#pragma once

#include <edbr/Graphics/Color.h>
#include <edbr/Graphics/Sprite.h>
#include <edbr/UI/Element.h>

struct Font;

namespace ui
{

class ImageElement : public Element {
public:
    ImageElement(const GPUImage& image, math::IntRect textureRect = {});

    void calculateOwnSize() override;

    Sprite sprite;
};

} // end of namespace ui
