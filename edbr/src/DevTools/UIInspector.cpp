#include <edbr/DevTools/UIInspector.h>

#include <edbr/Graphics/SpriteRenderer.h>
#include <edbr/Util/Palette.h>

#include <edbr/DevTools/ImGuiPropertyTable.h>
#include <edbr/UI/Element.h>
#include <edbr/Util/ImGuiUtil.h>
#include <edbr/Util/Palette.h>
#include <misc/cpp/imgui_stdlib.h>

#include <edbr/UI/ListLayoutElement.h>
#include <edbr/UI/TextElement.h>

#include <fmt/format.h>

#ifdef __GNUG__
#include <cxxabi.h>
#include <memory>
#endif

namespace
{

std::string getElementTypeName(const ui::Element& element)
{
    const auto tName = typeid(element).name();
    auto name = tName;
#ifdef __GNUG__
    int status;
    std::unique_ptr<char, void (*)(void*)>
        res{abi::__cxa_demangle(tName, NULL, NULL, &status), std::free};
    name = res.get();
#endif
    return name;
}

} // end of anonymous namespace

void UIInspector::showUITree(const ui::Element& element)
{
    const auto typeName = getElementTypeName(element);
    const auto nodeLabel =
        element.tag.empty() ? typeName : fmt::format("{} ({})", element.tag, typeName);

    ImGui::PushID((void*)&element);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                               ImGuiTreeNodeFlags_OpenOnDoubleClick |
                               ImGuiTreeNodeFlags_DefaultOpen;
    if (element.children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (&element == selectedUIElement) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    auto textColor = util::pickRandomColor(util::LightColorsPalette, (void*)&element);
    ImGui::PushStyleColor(ImGuiCol_Text, util::toImVec4(textColor));
    bool isOpen = ImGui::TreeNodeEx(nodeLabel.c_str(), flags);
    ImGui::PopStyleColor();
    if (isOpen) {
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            selectedUIElement = &element;
        }
        for (const auto& child : element.children) {
            showUITree(*child);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void UIInspector::showSelectedElementInfo()
{
    using namespace devtools;

    BeginPropertyTable();

    assert(hasSelectedElement());
    const auto& element = *selectedUIElement;

    DisplayProperty("Type", getElementTypeName(element));
    DisplayProperty("Absolute position", element.absolutePosition);
    DisplayProperty("Absolute size", element.absoluteSize);

    DisplayProperty("Relative position", element.relativePosition);
    DisplayProperty("Relative size", element.relativeSize);

    if (element.offsetPosition != glm::vec2{}) {
        DisplayProperty("Offset position", element.offsetPosition);
    }
    if (element.offsetSize != glm::vec2{}) {
        DisplayProperty("Offset size", element.offsetSize);
    }
    if (element.origin != glm::vec2{}) {
        DisplayProperty("Origin", element.origin);
    }
    if (element.fixedSize != glm::vec2{}) {
        DisplayProperty("Fixed size", element.fixedSize);
    }
    if (element.autoSize) {
        DisplayProperty("Auto size", element.autoSize);
        DisplayProperty("Auto size child idx", element.autoSizeChildIdx);
    }
    EndPropertyTable();

    BeginPropertyTable();
    if (auto lle = dynamic_cast<const ui::ListLayoutElement*>(&element); lle) {
        const auto dirStr = (lle->direction == ui::ListLayoutElement::Direction::Horizontal) ?
                                "Horizontal" :
                                "Vertical";
        DisplayProperty("Direction", dirStr);
        DisplayProperty("Padding", lle->padding);
        DisplayProperty("Auto size children", lle->autoSizeChildren);
    }

    if (auto tl = dynamic_cast<const ui::TextElement*>(&element); tl) {
        DisplayProperty("Text", tl->text);
        DisplayProperty("Color", tl->color);
    }

    EndPropertyTable();
}

void UIInspector::drawBoundingBoxes(SpriteRenderer& spriteRenderer, const ui::Element& element)
    const
{
    const auto bb = math::FloatRect{element.absolutePosition, element.absoluteSize};

    auto bbColor =
        edbr::rgbToLinear(util::pickRandomColor(util::LightColorsPalette, (void*)&element));
    bbColor.a = 0.25f;
    spriteRenderer.drawFilledRect(bb, bbColor);
    bbColor.a = 1.f;
    spriteRenderer.drawInsetRect(bb, bbColor);

    for (const auto& childPtr : element.children) {
        drawBoundingBoxes(spriteRenderer, *childPtr);
    }
}

void UIInspector::drawSelectedElement(SpriteRenderer& spriteRenderer) const
{
    assert(hasSelectedElement());
    const auto& element = *selectedUIElement;

    const auto bb = math::FloatRect{element.absolutePosition, element.absoluteSize};
    auto bbColor =
        edbr::rgbToLinear(util::pickRandomColor(util::LightColorsPalette, (void*)&element));

    { // simple way to "flash" the selected UI element
        static int frameNum = 0.f;
        ++frameNum;
        float v = std::abs(std::sin((float)frameNum / 60.f));
        bbColor.r *= v;
        bbColor.g *= v;
        bbColor.b *= v;
        bbColor.a = 0.5f;
    }

    spriteRenderer.drawFilledRect(bb, bbColor);
}
