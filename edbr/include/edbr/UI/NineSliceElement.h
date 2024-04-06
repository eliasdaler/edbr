#pragma once

#include <glm/vec2.hpp>

#include <edbr/UI/Element.h>
#include <edbr/UI/NineSlice.h>

namespace ui
{
struct NineSliceStyle;

class NineSliceElement : public Element {
public:
    NineSliceElement(const NineSliceStyle& style, const glm::vec2& size);

    const ui::NineSlice& getNineSlice() const { return nineSlice; }
    const glm::vec2& getSize() const { return size; }

private:
    ui::NineSlice nineSlice;
    glm::vec2 size;
};

} // end of namespace ui
