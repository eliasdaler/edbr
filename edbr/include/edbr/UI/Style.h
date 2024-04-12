#pragma once

#include <filesystem>

#include <edbr/Core/JsonDataLoader.h>
#include <edbr/Graphics/IdTypes.h>

class GfxDevice;

namespace ui
{
struct NineSliceStyle {
    std::filesystem::path textureFilename;
    ImageId texture{NULL_IMAGE_ID};

    math::IntRect textureRect;
    math::IntRect contentsTextureRect;

    struct TextureCoords {
        math::IntRect topLeftCorner;
        math::IntRect topBorder;
        math::IntRect topRightCorner;
        math::IntRect leftBorder;
        math::IntRect contents;
        math::IntRect rightBorder;
        math::IntRect bottomLeftCorner;
        math::IntRect bottomBorder;
        math::IntRect bottomRightBorder;
    };
    TextureCoords textureCoords;

    void load(const JsonDataLoader& loader, GfxDevice& gfxDevice);
};

} // end of namespace ui
