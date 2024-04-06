#include <edbr/UI/ListLayoutElement.h>

namespace ui
{
void ListLayoutElement::applyLayout()
{
    totalSize = {};

    auto currentPos = getPosition();
    for (const auto& c : children) {
        c->setPosition(currentPos);

        const auto childSize = c->getSize();
        if (direction == Direction::Horizontal) {
            currentPos.x += c->getSize().x;

            totalSize.x += c->getSize().x;
            totalSize.y = std::max(c->getSize().y, totalSize.y);
        } else {
            currentPos.y += c->getSize().y;

            totalSize.x = std::max(childSize.x, totalSize.x);
            totalSize.y += childSize.y;
        }
    }
}
}
