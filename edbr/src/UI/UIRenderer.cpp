#include <edbr/UI/UIRenderer.h>

#include <edbr/UI/ListLayoutElement.h>
#include <edbr/UI/PaddingElement.h>
#include <edbr/UI/TextButton.h>

#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/SpriteRenderer.h>

namespace ui
{

void UIRenderer::drawElement(
    SpriteRenderer& spriteRenderer,
    const Element& element,
    const glm::vec2& parentPos) const
{
    if (auto ns = dynamic_cast<const NineSliceElement*>(&element); ns) {
        ns->getNineSlice().draw(spriteRenderer, parentPos + element.getPosition(), ns->getSize());
    }

    if (auto tl = dynamic_cast<const ui::TextLabel*>(&element); tl) {
        auto p = tl->getPadding();
        auto textPos = parentPos + element.getPosition() + glm::vec2{p.left, p.top} +
                       glm::vec2{0.f, tl->getFont().lineSpacing};
        spriteRenderer.drawText(tl->getFont(), tl->getText(), textPos, tl->getColor());
    }

    for (const auto& childPtr : element.getChildren()) {
        drawElement(spriteRenderer, *childPtr, parentPos + element.getPosition());
    }
}

} // end of namespace ui
