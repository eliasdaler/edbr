#pragma once

#include <string>

#include <edbr/Graphics/Color.h>
#include <edbr/UI/Element.h>

struct Font;

namespace ui
{

class TextElement : public Element {
public:
    TextElement(const Font& font, const LinearColor& color = LinearColor::Black());
    TextElement(
        std::string text,
        const Font& font,
        const LinearColor& color = LinearColor::Black());

    void calculateOwnSize() override;
    // Sets fixed size which will be returned by calculateOwnSize
    // For example, setFixedSize("WWWWWW")
    // One of the problems with this is that the text will always get
    // left aligned inside the vounding box of maxText string.
    void setFixedSize(const std::string& maxText);

    // data
    std::string text;
    const Font& font;
    LinearColor color;

    bool shadow{false};
    LinearColor shadowColor{LinearColor::Black()};

    // Max glyphs to draw when drawing text (used for drawing only a part of text)
    // It's okay to have numGlyphsToDraw > text.size()
    int numGlyphsToDraw{std::numeric_limits<int>::max()};
};

} // end of namespace ui
