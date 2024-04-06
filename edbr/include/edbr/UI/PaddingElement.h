#pragma once

#include <glm/vec2.hpp>

#include <edbr/UI/Element.h>

namespace ui
{

class PaddingElement : public Element {
public:
    PaddingElement(const glm::vec2& padding = {}) : padding(padding)
    {
        assert(padding.x >= 0.f);
        assert(padding.y >= 0.f);
    }

    glm::vec2 getContentSize() const override { return padding; }

private:
    glm::vec2 padding;
};

} // end of namespace ui
