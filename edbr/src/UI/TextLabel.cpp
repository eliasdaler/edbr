#include <edbr/UI/TextLabel.h>

#include <edbr/Graphics/Font.h>

namespace ui
{

TextLabel::TextLabel(std::string text, const Font& font, const LinearColor& color) :
    text(std::move(text)), font(font), color(color)
{
    assert(font.loaded);
    boundingBox = font.calculateTextBoundingBox(this->text);
}

void TextLabel::setText(std::string text)
{
    this->text = std::move(text);
    boundingBox = font.calculateTextBoundingBox(this->text);
}

void TextLabel::setPadding(const Padding& p)
{
    assert(p.left >= 0.f);
    assert(p.right >= 0.f);
    assert(p.top >= 0.f);
    assert(p.bottom >= 0.f);
    padding = p;
}

glm::vec2 TextLabel::getSizeImpl() const
{
    auto bbSize = boundingBox.getSize();

    int numLines = 1;
    for (const auto& c : text) {
        if (c == '\n') {
            ++numLines;
        }
    }
    bbSize.y = numLines * font.lineSpacing;

    return bbSize + glm::vec2{padding.left + padding.right, padding.top + padding.bottom};
}

} // end of namespace ui
