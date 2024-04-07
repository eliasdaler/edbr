#pragma once

#include <glm/vec2.hpp>

class SpriteRenderer;

namespace ui
{

class Element;

class UIRenderer {
public:
    void drawElement(
        SpriteRenderer& spriteRenderer,
        const Element& element,
        const glm::vec2& parentPos = {}) const;

private:
};

} // end of namespace ui
