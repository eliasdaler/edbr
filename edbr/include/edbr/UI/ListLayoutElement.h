#pragma once

#include <edbr/UI/Element.h>

namespace ui
{

class ListLayoutElement : public Element {
public:
    enum class Direction { Horizontal, Vertical };

public:
    ListLayoutElement(Direction d = Direction::Horizontal) : direction(d) {}

    void applyLayout();

    glm::vec2 getContentSize() const override { return totalSize; }

private:
    Direction direction;
    glm::vec2 totalSize{};
};

} // end of namespace ui
