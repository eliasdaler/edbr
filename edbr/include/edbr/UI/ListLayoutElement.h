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
    void applyLayoutCentered(float parentWidth);

    glm::vec2 getSizeImpl() const override { return totalSize; }

    Direction getDirection() const { return direction; }

    // if set to true, child elements are centered proportionaly, e.g.
    // --[-A|A-]--[-B|B-]--
    void setCenteredPositioning(bool b) { centeredPositoning = b; }

private:
    Direction direction;
    glm::vec2 totalSize{};
    bool centeredPositoning{false};
};

} // end of namespace ui
