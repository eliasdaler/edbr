#pragma once

#include <edbr/UI/Element.h>
#include <edbr/UI/NineSlice.h>

namespace ui
{
struct NineSliceStyle;

class NineSliceElement : public Element {
public:
    NineSliceElement(const NineSliceStyle& style);

    const ui::NineSlice& getNineSlice() const { return nineSlice; }

private:
    ui::NineSlice nineSlice;
};

} // end of namespace ui
