#pragma once

#include <edbr/UI/Element.h>

namespace ui
{

struct ListLayoutElement : public Element {
    void calculateOwnSize() override;
    void calculateChildrenSizes() override;
    void calculateChildrenPositions() override;

    // data
    enum class Direction {
        Vertical,
        Horizontal,
    };

    Direction direction{Direction::Vertical};

    float padding{0.f}; // distance between elements

    // if true - all children will have the same width/height (of the widest/tallest element)
    bool autoSizeChildren{true};
};

} // end of namespace ui
