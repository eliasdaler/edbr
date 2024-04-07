#pragma once

#include <edbr/UI/Style.h>

class SpriteRenderer;

namespace ui
{
class NineSlice {
public:
    void setStyle(NineSliceStyle s);

    void draw(SpriteRenderer& renderer, const glm::vec2& position, const glm::vec2& size) const;

private:
    ui::NineSliceStyle style;
};

} // end of namespace ui