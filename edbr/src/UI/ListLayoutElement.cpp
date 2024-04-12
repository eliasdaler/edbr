#include <edbr/UI/ListLayoutElement.h>

#include <glm/common.hpp>

namespace ui
{

void ListLayoutElement::calculateOwnSize()
{
    glm::vec2 totalSize{};
    for (const auto& child : children) {
        child->calculateOwnSize();

        if (direction == Direction::Vertical) {
            totalSize.x = std::max(totalSize.x, child->absoluteSize.x);
            totalSize.y += child->absoluteSize.y + padding;
        } else {
            totalSize.y = std::max(totalSize.y, child->absoluteSize.y);
            totalSize.x += child->absoluteSize.x + padding;
        }
    }

    absoluteSize = totalSize;
}

void ListLayoutElement::calculateChildrenSizes()
{
    for (auto& child : children) {
        child->calculateSizes();

        if (autoSizeChildren) {
            auto cs = child->absoluteSize;
            if (direction == Direction::Vertical) {
                cs.x = absoluteSize.x;
            } else {
                cs.y = absoluteSize.y;
            }
            child->absoluteSize = cs;
            if (pixelPerfect) {
                child->absoluteSize = glm::round(child->absoluteSize);
            }
            // need to refresh sizes again as the size changed
            child->calculateChildrenSizes();
        }
    }
}

void ListLayoutElement::calculateChildrenPositions()
{
    auto currentPos = absolutePosition;
    for (auto& child : children) {
        child->absolutePosition = currentPos;
        if (pixelPerfect) {
            child->absolutePosition = glm::round(child->absolutePosition);
        }

        // Can't call child.calculateSizes here because it will call
        // child.calculateOwnSize and override the position we just calculated
        child->calculateChildrenPositions();

        if (direction == Direction::Vertical) {
            currentPos.y += child->absoluteSize.y + padding;
        } else {
            currentPos.x += child->absoluteSize.x + padding;
        }
    }
}

} // end of namespace ui
