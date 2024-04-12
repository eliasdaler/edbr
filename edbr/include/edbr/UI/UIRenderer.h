#pragma once

struct SpriteRenderer;

namespace ui
{
struct Element;

void drawElement(SpriteRenderer& spriteRenderer, const ui::Element& element);
} // end of namespace ui
