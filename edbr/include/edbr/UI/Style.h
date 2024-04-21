#pragma once

#include <filesystem>

#include <edbr/Core/JsonDataLoader.h>
#include <edbr/Graphics/IdTypes.h>

class GfxDevice;
struct Font;

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

struct ButtonStyle {
    const NineSliceStyle* nineSliceStyle{nullptr}; // if null - the button will have invisible
                                                   // border
    const Font& font;

    LinearColor normalColor; // when not selected
    LinearColor selectedColor;
    LinearColor disabledColor;

    enum class TextAlignment { Left, Center };
    TextAlignment textAlignment{TextAlignment::Center};

    glm::vec2 padding; // padding of text around the nine-patch/invisible frame
    glm::vec2 cursorOffset; // cursor offset relative to top-left corner of the button
};

} // end of namespace ui
