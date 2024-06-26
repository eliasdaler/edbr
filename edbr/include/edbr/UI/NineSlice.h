#pragma once

#include <edbr/UI/Style.h>

class GfxDevice;
class SpriteRenderer;

namespace ui
{
class NineSlice {
public:
    void setStyle(NineSliceStyle s);
    const NineSliceStyle& getStyle() const { return style; }

    void draw(
        GfxDevice& gfxDevice,
        SpriteRenderer& renderer,
        const glm::vec2& position,
        const glm::vec2& size) const;

private:
    ui::NineSliceStyle style;
};

} // end of namespace ui
