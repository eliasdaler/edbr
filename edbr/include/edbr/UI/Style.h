#pragma once

#include <filesystem>

#include <edbr/Core/JsonDataLoader.h>
#include <edbr/Graphics/IdTypes.h>

class GfxDevice;
struct Font;

namespace ui
{

struct ElementPositionAndSize {
    void load(const JsonDataLoader& loader);

    glm::vec2 relativePosition;
    glm::vec2 relativeSize{1.f};
    glm::vec2 offsetPosition;
    glm::vec2 offsetSize;
    glm::vec2 origin;
};

struct NineSliceStyle {
    void load(const JsonDataLoader& loader, GfxDevice& gfxDevice);

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
};

struct ButtonStyle {
    void load(const JsonDataLoader& loader);

    const NineSliceStyle* nineSliceStyle{nullptr}; // if null - the button will have invisible
                                                   // border

    LinearColor normalColor; // when not selected
    LinearColor selectedColor;
    LinearColor disabledColor;

    enum class TextAlignment { Left, Center };
    TextAlignment textAlignment{TextAlignment::Center};

    glm::vec2 padding; // padding of text around the nine-patch/invisible frame
    glm::vec2 cursorOffset; // cursor offset relative to top-left corner of the button
};

struct FontStyle {
    void load(const JsonDataLoader& loader);

    std::filesystem::path path;
    int size{16};
    bool antialiasing{false};
    float lineSpacing{0.f}; // if not 0 - custom line spacing is used
};

} // end of namespace ui
