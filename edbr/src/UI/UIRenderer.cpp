#include <edbr/UI/UIRenderer.h>

#include <edbr/UI/Element.h>
#include <edbr/UI/ImageElement.h>
#include <edbr/UI/NineSliceElement.h>
#include <edbr/UI/RectElement.h>
#include <edbr/UI/TextElement.h>

#include <edbr/Graphics/SpriteRenderer.h>

namespace ui
{
struct Element;

void drawElement(GfxDevice& gfxDevice, SpriteRenderer& spriteRenderer, const ui::Element& element)
{
    if (!element.visible) {
        return;
    }

    if (auto r = dynamic_cast<const RectElement*>(&element); r) {
        spriteRenderer.drawFilledRect(
            gfxDevice, {element.absolutePosition, element.absoluteSize}, r->fillColor);
    }

    if (auto ns = dynamic_cast<const NineSliceElement*>(&element); ns) {
        ns->getNineSlice()
            .draw(gfxDevice, spriteRenderer, element.absolutePosition, element.absoluteSize);
    }

    if (auto ts = dynamic_cast<const TextElement*>(&element); ts) {
        if (ts->shadow) {
            spriteRenderer.drawText(
                gfxDevice,
                ts->font,
                ts->text,
                element.absolutePosition + glm::vec2{0.f, 1.f},
                ts->shadowColor,
                ts->numGlyphsToDraw);
        }
        spriteRenderer.drawText(
            gfxDevice,
            ts->font,
            ts->text,
            element.absolutePosition,
            ts->color,
            ts->numGlyphsToDraw);
    }

    if (auto is = dynamic_cast<const ImageElement*>(&element); is) {
        spriteRenderer.drawSprite(gfxDevice, is->sprite, element.absolutePosition);
    }

    for (const auto& child : element.children) {
        drawElement(gfxDevice, spriteRenderer, *child);
    }
}

} // end of namespace ui
