#pragma once

#include <glm/vec2.hpp>

#include <edbr/UI/Element.h>
#include <edbr/UI/NineSlice.h>

namespace ui
{
struct NineSliceStyle;

class NineSliceElement : public Element {
public:
    NineSliceElement(const NineSliceStyle& style, const glm::vec2& size = {});

    const ui::NineSlice& getNineSlice() const { return nineSlice; }

    glm::vec2 getSizeImpl() const override;

    glm::vec2 getContentSize() const;

    math::FloatRect getBoundingBox() const override;

private:
    ui::NineSlice nineSlice;

    glm::vec2 contentSize;
};

} // end of namespace ui
