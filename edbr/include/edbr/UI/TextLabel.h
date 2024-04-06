#pragma once

#include <string>

#include <edbr/Math/Rect.h>
#include <edbr/UI/Element.h>

struct Font;

namespace ui
{

class TextLabel : public Element {
public:
    struct Padding {
        float left{0.f};
        float right{0.f};
        float top{0.f};
        float bottom{0.f};
    };

public:
    TextLabel(std::string text, const Font& font);

    void setText(std::string text);
    const std::string& getText() const { return text; }
    const Font& getFont() const { return font; }
    const math::FloatRect& getBoundingBox() { return boundingBox; }
    void setPadding(const Padding& p);
    const Padding& getPadding() const { return padding; }

    glm::vec2 getContentSize() const override;

private:
    std::string text;
    const Font& font;
    math::FloatRect boundingBox;
    Padding padding;
};

} // end of namespace ui
