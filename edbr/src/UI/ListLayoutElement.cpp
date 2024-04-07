#include <edbr/UI/ListLayoutElement.h>

#include <cmath>

namespace ui
{
void ListLayoutElement::applyLayout()
{
    assert(!centeredPositoning && "call applyLayoutCentered instead");

    totalSize = {};

    auto currentPos = glm::vec2{};
    for (const auto& c : children) {
        c->setPosition(currentPos);

        const auto childSize = c->getSize();
        if (direction == Direction::Horizontal) {
            currentPos.x += childSize.x;

            totalSize.x += childSize.x;
            totalSize.y = std::max(childSize.y, totalSize.y);
        } else {
            currentPos.y += c->getSize().y;

            totalSize.x = std::max(childSize.x, totalSize.x);
            totalSize.y += childSize.y;
        }
    }
}

void ListLayoutElement::applyLayoutCentered(float parentWidth)
{
    assert(direction == Direction::Horizontal);

    const auto w = parentWidth / (children.size() + 1);
    totalSize = {parentWidth, 0.f};
    for (std::size_t i = 0; i < children.size(); ++i) {
        auto& c = *children[i];
        const auto childSize = c.getSize();
        c.setPosition(glm::vec2{std::floor((i + 1) * w - childSize.x / 2.f), 0.f});

        totalSize.y = std::max(childSize.y, totalSize.y);
    }
}

}
