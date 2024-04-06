#pragma once

#include <string>

#include <edbr/Math/Rect.h>
#include <edbr/UI/Element.h>

struct Font;

namespace ui
{

class TextLabel : public Element {
public:
    TextLabel(std::string text, const Font& font);

    void setText(std::string text);
    const std::string& getText() const { return text; }
    const Font& getFont() const { return font; }
    const math::FloatRect& getBoundingBox() { return boundingBox; }

private:
    std::string text;
    const Font& font;
    math::FloatRect boundingBox;
};

} // end of namespace ui
