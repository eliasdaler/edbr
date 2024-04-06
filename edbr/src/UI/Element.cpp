#include <edbr/UI/Element.h>

#include <cassert>

namespace ui
{
void Element::addChild(std::unique_ptr<Element> element)
{
    assert(!element->parent);
    children.push_back(std::move(element));
}

glm::vec2 Element::getSize() const
{
    auto size = getContentSize();
    if (autoSizing == AutomaticSizing::None) {
        return size;
    }

    assert(children.size() == 1); // TODO: what to do with multiple children?
    const auto& child = children[0];
    const auto childContentSize = child->getContentSize();

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

} // end of namespace ui
