#pragma once

class GfxDevice;
class SpriteRenderer;

namespace ui
{
struct Element;

void drawElement(GfxDevice& gfxDevice, SpriteRenderer& spriteRenderer, const ui::Element& element);
} // end of namespace ui
