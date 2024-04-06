#pragma once

#include <edbr/UI/Style.h>

class SpriteRenderer;

namespace ui
{
class NineSlice {
public:
    void setStyle(NineSliceStyle s);

    void setPosition(const glm::vec2& pos) { position = pos; }
    const glm::vec2& getPosition() const { return position; }

    void setSize(const glm::vec2& newSize);
    const glm::vec2& getSize() const { return size; }

    void draw(SpriteRenderer& renderer) const;

private:
    glm::vec2 position;
    glm::vec2 size;

    ui::NineSliceStyle style;
};

} // end of namespace ui
