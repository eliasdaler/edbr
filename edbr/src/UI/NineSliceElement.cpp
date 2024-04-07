#include <edbr/UI/NineSliceElement.h>

namespace ui
{

namespace
{
    glm::vec2 getCornerSize(const NineSliceStyle& style)
    {
        return static_cast<glm::vec2>(style.coords.corner.getSize());
    }
}

NineSliceElement::NineSliceElement(const NineSliceStyle& style, const glm::vec2& size) :
    contentSize(size)
{
    if (size == glm::vec2{}) {
        setAutomaticSizing(AutomaticSizing::XY);
    } else {
        assert(size.x != 0.f && size.y != 0.f);
        // can potentially set AutomaticSizing to X if size.x == 0, and Y if size.y == 0
    }

    nineSlice.setStyle(style);
}

glm::vec2 NineSliceElement::getContentSize() const
{
    if (autoSizing == AutomaticSizing::None) {
        return contentSize;
    }
    assert(autoSizing == AutomaticSizing::XY); // support XY for now only
    const auto& child = children[autoSizeElementIndex];
    const auto childContentSize = child->getSize();
    return child->getSize();
}

glm::vec2 NineSliceElement::getSizeImpl() const
{
    if (autoSizing == AutomaticSizing::None) {
        return contentSize + getCornerSize(nineSlice.getStyle()) * 2.f;
    }
    assert(autoSizing == AutomaticSizing::XY); // support XY for now only
    const auto& child = children[autoSizeElementIndex];
    const auto childContentSize = child->getSize();
    return childContentSize + getCornerSize(nineSlice.getStyle()) * 2.f;
}

math::FloatRect NineSliceElement::getBoundingBox() const
{
    return {-getCornerSize(nineSlice.getStyle()), getSize()};
}

} // end of namespace ui
