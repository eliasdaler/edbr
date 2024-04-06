#include <edbr/UI/TextLabel.h>

#include <edbr/Graphics/Font.h>

namespace ui
{

TextLabel::TextLabel(std::string text, const Font& font) : text(std::move(text)), font(font)
{
    assert(font.loaded);
    boundingBox = font.calculateTextBoundingBox(text);
}

void TextLabel::setText(std::string text)
{
    this->text = std::move(text);
    boundingBox = font.calculateTextBoundingBox(this->text);
}

} // end of namespace ui
