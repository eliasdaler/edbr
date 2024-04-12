#pragma once

#include <edbr/Graphics/Color.h>
#include <edbr/UI/Element.h>

namespace ui
{

struct RectElement : Element {
    LinearColor fillColor{LinearColor::White()};
};

} // end of namespace ui
