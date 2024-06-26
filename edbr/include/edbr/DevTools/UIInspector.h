#pragma once

#include <glm/vec2.hpp>

#include <edbr/Graphics/Color.h>

class GfxDevice;
class SpriteRenderer;

namespace ui
{
struct Element;
}

class UIInspector {
public:
    void setInspectedUI(const ui::Element& element);

    void updateUI(float dt);

    void draw(GfxDevice& gfxDevice, SpriteRenderer& spriteRenderer) const;

    bool drawUIElementBoundingBoxes{false};

    void deselectSelectedElement() { selectedUIElement = nullptr; }
    bool hasSelectedElement() const { return selectedUIElement != nullptr; }
    const ui::Element& getSelectedElement() const { return *selectedUIElement; }

    void setFocusElement(const ui::Element& element) { focusUIElement = &element; }

private:
    void showUITree(const ui::Element& element, bool openByDefault = true);
    void showSelectedElementInfo();
    void drawBoundingBoxes(
        GfxDevice& gfxDevice,
        SpriteRenderer& spriteRenderer,
        const ui::Element& element) const;
    void drawSelectedElement(GfxDevice& gfxDevice, SpriteRenderer& spriteRenderer) const;
    void drawBorderAroundElement(
        GfxDevice& gfxDevice,
        SpriteRenderer& spriteRenderer,
        const ui::Element& element,
        const LinearColor& color,
        float borderWidth = 4.f) const;

    const ui::Element* inspectedUIElement{nullptr};
    const ui::Element* selectedUIElement{nullptr};
    const ui::Element* focusUIElement{nullptr};
};
