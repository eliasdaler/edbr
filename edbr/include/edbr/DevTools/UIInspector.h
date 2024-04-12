#pragma once

#include <glm/vec2.hpp>

class SpriteRenderer;

namespace ui
{
class Element;
}

class UIInspector {
public:
    void showUITree(const ui::Element& element);
    void showSelectedElementInfo();

    void drawBoundingBoxes(SpriteRenderer& spriteRenderer, const ui::Element& element) const;

    void drawSelectedElement(SpriteRenderer& spriteRenderer) const;

    bool drawUIElementBoundingBoxes{false};

    void deselectSelectedElement() { selectedUIElement = nullptr; }
    bool hasSelectedElement() const { return selectedUIElement != nullptr; }

private:
    const ui::Element* selectedUIElement{nullptr};
};
