#include <edbr/UI/TextElement.h>

#include <edbr/Graphics/Font.h>

#include <glm/common.hpp>

namespace ui
{

TextElement::TextElement(const Font& font, const LinearColor& color) : font(font), color(color)
{
    assert(font.loaded);
}

TextElement::TextElement(std::string text, const Font& font, const LinearColor& color) :
    text(std::move(text)), font(font), color(color)
{
    assert(font.loaded);
}

void TextElement::setFixedSize(const std::string& maxText)
{
    fixedSize = font.calculateTextBoundingBox(maxText).getSize();
}

void TextElement::calculateOwnSize()
{
    absoluteSize = font.calculateTextBoundingBox(text).getSize();
    if (fixedSize != glm::vec2{}) {
        absoluteSize = fixedSize;
    }
    if (pixelPerfect) {
        absoluteSize = glm::round(absoluteSize);
    }
}

} // end of namespace ui
