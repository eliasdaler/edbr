#include <edbr/UI/NineSliceElement.h>

namespace ui
{

NineSliceElement::NineSliceElement(const NineSliceStyle& style, const glm::vec2& size) : size(size)
{
    nineSlice.setStyle(style);
}

} // end of namespace ui
