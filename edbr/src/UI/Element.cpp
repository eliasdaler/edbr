#include <edbr/UI/Element.h>

#include <glm/common.hpp>

namespace ui
{

Element& Element::addChild(std::unique_ptr<Element> child)
{
    assert(child.get() != this);
    assert(!child->parent);
    child->parent = this;
    children.push_back(std::move(child));
    return *children.back();
}

Element* Element::tryFindElement(const std::string& tag)
{
    if (this->tag == tag) {
        return this;
    }
    for (const auto& c : children) {
        if (auto e = c->tryFindElement(tag); e) {
            return e;
        }
    }
    return nullptr;
}

void Element::calculateLayout(const glm::vec2& screenSize)
{
    assert(!parent && "calculateLayout should be called on a root UI element");
    calculateSizes();

    // calculate position based on screen size
    absolutePosition = relativePosition * screenSize + offsetPosition - origin * absoluteSize;
    if (pixelPerfect) {
        absolutePosition = glm::round(absolutePosition);
    }

    calculatePositions();
}

void Element::calculateSizes()
{
    calculateOwnSize();
    calculateChildrenSizes();
}

void Element::calculateOwnSize()
{
    if (autoSize) {
        // autosize based on the child's size
        assert(autoSizeChildIdx + 1 <= children.size());
        auto& child = *children[autoSizeChildIdx];
        child.calculateSizes();

        // Calculate parent's size based on child's
        absoluteSize = child.absoluteSize / child.relativeSize - child.offsetSize;
    } else {
        if (!parent) {
            throw std::runtime_error(
                "root element can't calculate own size unless fixedSize or autoSize is set");
        }

        absoluteSize = relativeSize * parent->absoluteSize + offsetSize;
    }

    if (fixedSize != glm::vec2{}) {
        // this will override the previous calculations - we want this
        absoluteSize = fixedSize;
    }

    if (pixelPerfect) {
        absoluteSize = glm::round(absoluteSize);
    }
}

void Element::calculateChildrenSizes()
{
    // calculate other children sizes
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (autoSize && i == autoSizeChildIdx) {
            // already calculated during calculateOwnSize
            continue;
        }

        auto& child = *children[i];
        child.calculateSizes();
    }
}

void Element::calculatePositions()
{
    calculateOwnPosition();
    calculateChildrenPositions();
}

void Element::calculateOwnPosition()
{
    if (!parent) {
        // for the root element, the position is calculated in calculateLayout
        return;
    }
    // calculate based on parent's position
    absolutePosition = parent->absolutePosition + relativePosition * parent->absoluteSize +
                       offsetPosition - origin * absoluteSize;
    if (pixelPerfect) {
        absolutePosition = glm::round(absolutePosition);
    }
}

// calculate children positions
void Element::calculateChildrenPositions()
{
    for (auto& childPtr : children) {
        childPtr->calculatePositions();
    }
}

} // end of namespace ui
