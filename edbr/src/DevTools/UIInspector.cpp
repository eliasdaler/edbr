#include <edbr/DevTools/UIInspector.h>

#include <edbr/Graphics/SpriteRenderer.h>
#include <edbr/Util/Palette.h>

#include <edbr/DevTools/ImGuiPropertyTable.h>
#include <edbr/UI/Element.h>
#include <edbr/Util/ImGuiUtil.h>
#include <edbr/Util/Palette.h>
#include <misc/cpp/imgui_stdlib.h>

#include <edbr/UI/ListLayoutElement.h>
#include <edbr/UI/TextLabel.h>

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
        element.getTag().empty() ? typeName : fmt::format("{} ({})", element.getTag(), typeName);

    ImGui::PushID((void*)&element);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                               ImGuiTreeNodeFlags_OpenOnDoubleClick |
                               ImGuiTreeNodeFlags_DefaultOpen;
    if (element.getChildren().empty()) {
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
        for (const auto& child : element.getChildren()) {
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

    DisplayProperty("Position", element.getPosition());
    auto screenPos = element.calculateScreenPosition();
    DisplayProperty("Screen position", screenPos);

    auto bb = element.getBoundingBox();
    DisplayProperty("Bounding box", bb);
    bb.setPosition(screenPos + bb.getTopLeftCorner());
    DisplayProperty("Bounding box(screen)", bb);

    DisplayProperty("Size", element.getSize());

    if (element.getAutomaticSizing() != ui::Element::AutomaticSizing::None) {
        const auto automaticSizeStr = [](ui::Element::AutomaticSizing s) {
            switch (s) {
            case ui::Element::AutomaticSizing::None:
                return "None";
                break;
            case ui::Element::AutomaticSizing::X:
                return "X";
                break;
            case ui::Element::AutomaticSizing::Y:
                return "Y";
                break;
            case ui::Element::AutomaticSizing::XY:
                return "XY";
                break;
            default:
                return "???";
                break;
            }
        }(element.getAutomaticSizing());
        DisplayProperty("Auto size", automaticSizeStr);
    }

    if (auto lle = dynamic_cast<const ui::ListLayoutElement*>(&element); lle) {
        const auto dirStr = (lle->getDirection() == ui::ListLayoutElement::Direction::Horizontal) ?
                                "Horizontal" :
                                "Vertical";
        DisplayProperty("Direction", dirStr);
    }

    if (auto tl = dynamic_cast<const ui::TextLabel*>(&element); tl) {
        DisplayProperty("Text", tl->getText());
    }

    EndPropertyTable();
}

void UIInspector::drawBoundingBoxes(
    SpriteRenderer& spriteRenderer,
    const ui::Element& element,
    const glm::vec2& parentPos) const
{
    auto bb = element.getBoundingBox();
    auto selfPos = parentPos + element.getPosition();
    bb.setPosition(selfPos + bb.getTopLeftCorner());

    auto bbColor =
        edbr::rgbToLinear(util::pickRandomColor(util::LightColorsPalette, (void*)&element));
    bbColor.a = 0.25f;
    spriteRenderer.drawFilledRect(bb, bbColor);
    bbColor.a = 1.f;
    spriteRenderer.drawInsetRect(bb, bbColor);

    for (const auto& childPtr : element.getChildren()) {
        drawBoundingBoxes(spriteRenderer, *childPtr, selfPos);
    }
}

void UIInspector::drawSelectedElement(SpriteRenderer& spriteRenderer) const
{
    assert(hasSelectedElement());
    const auto& element = *selectedUIElement;

    auto pos = element.calculateScreenPosition();
    auto bb = element.getBoundingBox();
    bb.setPosition(pos + bb.getTopLeftCorner());
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
