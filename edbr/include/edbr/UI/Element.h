#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

#include <edbr/UI/InputEvent.h>

namespace ui
{

struct Element {
    virtual ~Element() {}

    Element& addChild(std::unique_ptr<Element> child);

    // Finds an element by tag, if not found - returns nullptr
    Element* tryFindElement(const std::string& tag);

    template<typename T>
    T& findElement(const std::string& tag)
    {
        auto ptr = tryFindElement(tag);
        assert(ptr && "UI element was not found");
        return static_cast<T&>(*ptr);
    }

    // Main layout calculation - should be called for each root UI element
    void calculateLayout(const glm::vec2& screenSize);

    // Calculate own and children sizes
    void calculateSizes();
    // Calculate own size
    virtual void calculateOwnSize();
    // Calculate children sizes
    virtual void calculateChildrenSizes();

    // Calculate own and children positions
    void calculatePositions();
    // Calculate children positions
    virtual void calculateChildrenPositions();
    // Calculate own position (based on screen size or parent)
    void calculateOwnPosition();

    void setSelected(bool b);
    // what happens when element becomes in focus or focus exits from it
    virtual void onSelected() { selected = true; };
    virtual void onDeselected() { selected = false; };

    void setEnabled(bool b);
    // what happens when element is disabled/enabled
    virtual void onEnabled(){};
    virtual void onDisabled(){};

    // data

    // Position relative to parent's size, e.g.:
    // (0.5, 0.5) - center of the parent
    // (1, 1) - bottom right corner of the parent
    glm::vec2 relativePosition{};

    // Size relative to the parent's, e.g.:
    // (1, 1) - same size as the parent's
    // (0.5, 0.5) - half of the parent's size
    glm::vec2 relativeSize{1.f};

    // Position offset in pixels, e.g.
    // offsetPosition of (x, y) will move children
    // absolute position x pixels right and y pixels down
    glm::vec2 offsetPosition{};

    // Size offset in pixels, e.g.
    // offsetSize of (w, h) will make the size of the child element
    // w pixels wider and h pixels taller
    glm::vec2 offsetSize{};

    // Origin/anchor of the element relative to its size, e.g.:
    // (0, 0) - top left corner, (0.5, 0.5) - center,
    // (1, 0.5) - bottom center, (1, 1) - bottom right corner
    // The element is positioned around it.
    glm::vec2 origin{};

    // Absolute position relative to the top-left corner of the screen
    // Read-only, computed by calculateLayout, should not be modified manually
    glm::vec2 absolutePosition{};

    // Absolute size in pixels
    // Read-only, computed by calculateLayout, should not be modified manually
    glm::vec2 absoluteSize{};

    // If not equal to glm::vec2{0}, calculateOwnPosition will return this.
    // Note that you'll need to return this from calculateOwnPosition if you override it
    glm::vec2 fixedSize{};

    // if set to true - size calculation will be performed based on the children content
    bool autoSize{false};
    // Index of the child whose size would be used for resizing the parent
    std::size_t autoSizeChildIdx{0};

    // If true, absolutePosition and absoluteSize is rounded during calculations
    bool pixelPerfect{true};

    // Hierarchy
    Element* parent{nullptr};
    std::vector<std::unique_ptr<Element>> children;

    // Tag used to find the element inside the hierarchy
    // should be unique for each element if non-empty
    std::string tag;

    bool visible{true};

    // if true - element accepts inputs, otherwise it's drawn, but doesn't accept inputs
    // call setEnabled(b) to set, don't set manually
    bool enabled{true};

    // call setSelected(b), don't set manually
    bool selected{false};

    // The first element that will get selected when the cursor enters the menu
    // Should be set on the root node only
    Element* cursorSelectionStartElement{nullptr};

    // which objects cursor navigates to when up/down/left right is pressed
    Element* cursorUpElement{nullptr};
    Element* cursorDownElement{nullptr};
    Element* cursorLeftElement{nullptr};
    Element* cursorRightElement{nullptr};

    InputHandlerType inputHandler;

    // Offset from absolute position to which point the cursor will point to
    glm::vec2 cursorSelectionOffset{};
};

} // end of namespace ui
