#include <edbr/UI/Element.h>

#include <cassert>

namespace ui
{
Element& Element::addChild(std::unique_ptr<Element> element)
{
    assert(!element->parent);
    element->parent = this;
    children.push_back(std::move(element));
    return *children.back();
}

glm::vec2 Element::getSize() const
{
    auto size = getSizeImpl();
    if (autoSizing == AutomaticSizing::None) {
        return size;
    }

    const auto& child = children[autoSizeElementIndex];
    const auto childContentSize = child->getSizeImpl();

    switch (autoSizing) {
    case AutomaticSizing::X:
        size.x = childContentSize.x;
        break;
    case AutomaticSizing::Y:
        size.y = childContentSize.y;
        break;
    case AutomaticSizing::XY:
        size.x = childContentSize.x;
        size.y = childContentSize.y;
        break;
    default:
        assert(false);
        break;
    }

    return size;
}

glm::vec2 Element::calculateScreenPosition() const
{
    auto pos = getPosition();
    const auto* currParent = getParent();
    while (currParent != nullptr) {
        pos += currParent->getPosition();
        currParent = currParent->getParent();
    }
    return pos;
}

void Element::setAutomaticSizingElementIndex(std::size_t i)
{
    assert(i + 1 <= children.size());
    autoSizeElementIndex = i;
}

} // end of namespace ui
