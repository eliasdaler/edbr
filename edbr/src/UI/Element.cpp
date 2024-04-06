#include <edbr/UI/Element.h>

#include <cassert>

namespace ui
{
void Element::addChild(std::unique_ptr<Element> element)
{
    assert(!element->parent);
    children.push_back(std::move(element));
}

} // end of namespace ui
