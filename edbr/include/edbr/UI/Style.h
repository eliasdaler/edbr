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

    struct TextureCoords {
        math::IntRect corner;
        math::IntRect horizonal;
        math::IntRect vertical;
        math::IntRect contents;
    } coords;

    void load(const JsonDataLoader& loader, GfxDevice& gfxDevice);
};

} // end of namespace ui
