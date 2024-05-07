#include <edbr/Graphics/GfxDevice.h>
#include <edbr/UI/Style.h>

namespace ui
{

void ElementPositionAndSize::load(const JsonDataLoader& loader)
{
    loader.getIfExists("relativePosition", relativePosition);
    loader.getIfExists("relativeSize", relativeSize);
    loader.getIfExists("offsetPosition", offsetPosition);
    loader.getIfExists("offsetSize", offsetSize);
    loader.getIfExists("origin", origin);
}

void NineSliceStyle::load(const JsonDataLoader& loader, GfxDevice& gfxDevice)
{
    // load texture
    loader.get("texture", textureFilename);
    texture = gfxDevice.loadImageFromFile(textureFilename);
    assert(texture != NULL_IMAGE_ID);

    if (loader.hasKey("textureCoords")) {
        const auto cl = loader.getLoader("textureCoords");
        if (cl.hasKey("rect")) {
            cl.get("rect", textureRect);
        } else {
            auto& tex = gfxDevice.getImage(texture);
            textureRect = {{}, tex.getSize2D()};
        }
        cl.get("contents", contentsTextureRect);

        // compute texture coords
        textureCoords.topLeftCorner = math::IntRect{
            textureRect.left,
            textureRect.top,
            contentsTextureRect.left - textureRect.left,
            contentsTextureRect.top - textureRect.top};

        textureCoords.topBorder = math::IntRect{
            contentsTextureRect.left,
            textureRect.top,
            contentsTextureRect.width,
            contentsTextureRect.top - textureRect.top};

        textureCoords.topRightCorner = math::IntRect{
            textureRect.left + contentsTextureRect.left + contentsTextureRect.width,
            textureRect.top,
            textureRect.left + textureRect.width -
                (contentsTextureRect.left + contentsTextureRect.width),
            contentsTextureRect.top - textureRect.top};

        textureCoords.leftBorder = math::IntRect{
            textureRect.left,
            contentsTextureRect.top,
            contentsTextureRect.left - textureRect.left,
            contentsTextureRect.height,
        };

        textureCoords.contents = contentsTextureRect;

        textureCoords.rightBorder = math::IntRect{
            contentsTextureRect.left + contentsTextureRect.width,
            contentsTextureRect.top,
            textureRect.left + textureRect.width -
                (contentsTextureRect.left + contentsTextureRect.width),
            contentsTextureRect.height,
        };

        textureCoords.bottomLeftCorner = math::IntRect{
            textureRect.left,
            textureRect.top + (contentsTextureRect.top + contentsTextureRect.height),
            contentsTextureRect.left - textureRect.left,
            textureRect.top + textureRect.height -
                (contentsTextureRect.top + contentsTextureRect.height),
        };

        textureCoords.bottomBorder = math::IntRect{
            contentsTextureRect.left,
            contentsTextureRect.top + contentsTextureRect.height,
            contentsTextureRect.width,
            textureRect.top + textureRect.height -
                (contentsTextureRect.top + contentsTextureRect.height),
        };

        textureCoords.bottomRightBorder = math::IntRect{
            contentsTextureRect.left + contentsTextureRect.width,
            contentsTextureRect.top + contentsTextureRect.height,
            textureRect.left + textureRect.width -
                (contentsTextureRect.left + contentsTextureRect.width),
            textureRect.top + textureRect.height -
                (contentsTextureRect.top + contentsTextureRect.height),
        };
    }
}

void ButtonStyle::load(const JsonDataLoader& loader)
{
    loader.get("normalColor", normalColor);
    loader.get("selectedColor", selectedColor);
    loader.get("disabledColor", disabledColor);

    std::string taString;
    loader.get("textAlignment", taString);
    if (taString == "Left") {
        textAlignment = TextAlignment::Left;
    } else if (taString == "Center") {
        textAlignment = TextAlignment::Center;
    } else {
        throw std::runtime_error(fmt::format("unknown alignment type '{}", taString));
    }

    loader.get("padding", padding);
    loader.get("cursorOffset", cursorOffset);
}

void FontStyle::load(const JsonDataLoader& loader)
{
    loader.get("path", path);
    loader.get("size", size);
    loader.getIfExists("aa", antialiasing);
}

}
