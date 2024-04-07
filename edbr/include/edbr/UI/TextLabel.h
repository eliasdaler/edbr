#pragma once

#include <string>

#include <edbr/Graphics/Color.h>
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
    TextLabel(std::string text, const Font& font, const LinearColor& color = LinearColor::Black());

    void setText(std::string text);
    const std::string& getText() const { return text; }
    const Font& getFont() const { return font; }
    math::FloatRect getBoundingBox() const override;
    void setPadding(const Padding& p);
    const Padding& getPadding() const { return padding; }

    glm::vec2 getSizeImpl() const override;

    void setColor(const LinearColor& c) { color = c; }
    const LinearColor& getColor() const { return color; }

private:
    std::string text;
    const Font& font;
    LinearColor color;
    math::FloatRect boundingBox;
    Padding padding;
};

} // end of namespace ui
