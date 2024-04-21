#include <gtest/gtest.h>

#include <edbr/UI/Element.h>

namespace glm
{
void PrintTo(const glm::vec2& v, std::ostream* os)
{
    *os << "glm::vec2{" << v.x << ", " << v.y << "}";
}
};

TEST(UILayout, TestAutoSizingBasic)
{
    // root element - centered around the screen center
    ui::Element element;
    element.origin = {0.5f, 0.5f};
    element.relativePosition = glm::vec2{0.5f, 0.5f};
    element.autoSize = true;

    // child element - with fixed size of {64.f, 32.f}
    // with size equal to the parent (the parent will take its size)
    auto child = std::make_unique<ui::Element>();
    const auto childSize = glm::vec2{64.f, 32.f};
    child->fixedSize = childSize;
    child->relativeSize = {1.f, 1.f};
    child->origin = {0.5f, 0.5f};

    element.addChild(std::move(child));

    const auto screenSize = glm::vec2{640, 480};
    element.calculateLayout(screenSize);

    EXPECT_EQ(element.children[0]->absoluteSize, childSize);
    EXPECT_EQ(element.children[0]->absolutePosition, (glm::vec2{256, 208}));

    EXPECT_EQ(element.absoluteSize, childSize);
    EXPECT_EQ(element.absolutePosition, screenSize / 2.f - childSize / 2.f);
}

TEST(UILayout, TestAutoSizingRelativePositionAndSize)
{
    // root element - centered around the screen center
    ui::Element element;
    element.origin = {0.5f, 0.5f};
    element.relativePosition = glm::vec2{0.5f, 0.5f};
    element.autoSize = true;

    // child element - with fixed size of {64.f, 32.f}
    // with size 0.5fx to the parent (the parent will take double its size)
    // and relativePos = (0.25f, 0.25f) so that it's centered inside of the parent
    auto child = std::make_unique<ui::Element>();
    const auto childSize = glm::vec2{64.f, 32.f};
    child->fixedSize = childSize;
    child->relativePosition = {0.25f, 0.25f};
    child->relativeSize = {0.5f, 0.5f};
    child->origin = {0.5f, 0.5f};

    element.addChild(std::move(child));

    const auto screenSize = glm::vec2{640, 480};
    element.calculateLayout(screenSize);

    EXPECT_EQ(element.children[0]->absoluteSize, childSize);
    EXPECT_EQ(element.children[0]->absolutePosition, (glm::vec2{256, 208}));

    EXPECT_EQ(element.absoluteSize, childSize * 2.f);
    EXPECT_EQ(element.absolutePosition, (glm::vec2{256, 208}));
}

TEST(UILayout, TestAutoSizingOffsetPositionAndSize)
{
    // root element - centered around the screen center
    ui::Element element;
    element.origin = {0.5f, 0.5f};
    element.relativePosition = glm::vec2{0.5f, 0.5f};
    element.autoSize = true;

    // same as the previous test, but now we also add position and size offset
    // to expand by 16px on each size
    auto child = std::make_unique<ui::Element>();
    const auto childSize = glm::vec2{64.f, 32.f};
    child->fixedSize = childSize;

    child->relativePosition = {0.25f, 0.25f};
    child->relativeSize = {0.5f, 0.5f};

    child->offsetPosition = {16.f, 8.f};
    child->offsetSize = {-32.f, -16.f};

    child->origin = {0.5f, 0.5f};

    element.addChild(std::move(child));

    const auto screenSize = glm::vec2{640, 480};
    element.calculateLayout(screenSize);

    EXPECT_EQ(element.children[0]->absoluteSize, childSize);
    EXPECT_EQ(element.children[0]->absolutePosition, (glm::vec2{264, 212}));

    EXPECT_EQ(element.absoluteSize, (childSize * 2.f + glm::vec2{32.f, 16.f}));
    EXPECT_EQ(element.absolutePosition, (glm::vec2{240, 200}));
}
