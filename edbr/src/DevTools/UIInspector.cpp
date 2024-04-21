#include <edbr/DevTools/UIInspector.h>

#include <edbr/Graphics/SpriteRenderer.h>
#include <edbr/Util/Palette.h>

#include <edbr/DevTools/ImGuiPropertyTable.h>
#include <edbr/UI/Element.h>
#include <edbr/Util/ImGuiUtil.h>
#include <edbr/Util/MetaUtil.h>
#include <edbr/Util/Palette.h>
#include <misc/cpp/imgui_stdlib.h>

#include <edbr/UI/ButtonElement.h>
#include <edbr/UI/ListLayoutElement.h>
#include <edbr/UI/TextElement.h>

#include <fmt/format.h>

namespace
{

std::string getElementTypeName(const ui::Element& element)
{
    return util::getDemangledTypename(typeid(element).name());
}

} // end of anonymous namespace

void UIInspector::setInspectedUI(const ui::Element& element)
{
    inspectedUIElement = &element;
}

void UIInspector::updateUI(float dt)
{
    if (inspectedUIElement) {
        showUITree(*inspectedUIElement);
    }
    if (hasSelectedElement()) {
        if (ImGui::Begin("Selected UI element")) {
            showSelectedElementInfo();
        }
        ImGui::End();
    }
}

void UIInspector::showUITree(const ui::Element& element, bool openByDefault)
{
    const auto typeName = getElementTypeName(element);
    auto nodeLabel = element.tag.empty() ? typeName : fmt::format("{} ({})", typeName, element.tag);

    // hack
    if (auto bl = dynamic_cast<const ui::ButtonElement*>(&element); bl) {
        openByDefault = false;
        nodeLabel += fmt::format(" ({})", bl->getTextElement().text);
    }

    ImGui::PushID((void*)&element);
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (element.children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (&element == selectedUIElement) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (openByDefault) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    auto textColor = util::pickRandomColor(util::LightColorsPalette, (void*)&element);
    ImGui::PushStyleColor(ImGuiCol_Text, util::toImVec4(textColor));
    bool isOpen = ImGui::TreeNodeEx(nodeLabel.c_str(), flags);
    ImGui::PopStyleColor();
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selectedUIElement = &element;
    }
    if (isOpen) {
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

    assert(hasSelectedElement());
    const auto& element = *selectedUIElement;

    BeginPropertyTable();
    DisplayProperty("Type", getElementTypeName(element));
    DisplayProperty("Visible", element.visible);
    DisplayProperty("Selected", element.selected);
    DisplayProperty("Enabled", element.enabled);
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

    focusUIElement = nullptr;
    auto displayHoverElement = [this](const char* name, ui::Element* element) {
        if (!element) {
            return;
        }
        DisplayPropertyCustomBegin(name);
        ImGui::PushID((void*)element);
        ImGui::Text("Hover to focus");
        if (ImGui::IsItemHovered()) {
            focusUIElement = element;
        }
        ImGui::PopID();
    };
    displayHoverElement("Selection start", element.cursorSelectionStartElement);
    displayHoverElement("Cursor up", element.cursorUpElement);
    displayHoverElement("Cursor down", element.cursorDownElement);
    displayHoverElement("Cursor left", element.cursorLeftElement);
    displayHoverElement("Cursor right", element.cursorRightElement);

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

    if (auto bl = dynamic_cast<const ui::ButtonElement*>(&element); bl) {
        DisplayProperty("Text", bl->getTextElement().text);
        DisplayProperty("Normal color", bl->normalColor);
        DisplayProperty("Selected color", bl->selectedColor);
        DisplayProperty("Disabled color", bl->disabledColor);
    }

    EndPropertyTable();
}

void UIInspector::draw(SpriteRenderer& spriteRenderer) const
{
    if (inspectedUIElement && drawUIElementBoundingBoxes) {
        drawBoundingBoxes(spriteRenderer, *inspectedUIElement);
    }
    if (selectedUIElement) {
        drawSelectedElement(spriteRenderer);
    }
    if (focusUIElement) {
        drawBorderAroundElement(
            spriteRenderer, *focusUIElement, LinearColor{1.f, 0.f, 0.f, 0.5f}, 4.f);
    }
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
    auto bbColor =
        edbr::rgbToLinear(util::pickRandomColor(util::LightColorsPalette, (void*)&element));

    { // simple way to "flash" the selected UI element
        static int frameNum = 0;
        ++frameNum;
        float v = std::abs(std::sin((float)frameNum / 60.f));
        bbColor.r *= v;
        bbColor.g *= v;
        bbColor.b *= v;
        bbColor.a = 0.5f;
    }
    drawBorderAroundElement(spriteRenderer, element, bbColor, 4.f);
}

void UIInspector::drawBorderAroundElement(
    SpriteRenderer& spriteRenderer,
    const ui::Element& element,
    const LinearColor& color,
    float borderWidth) const
{
    const auto bb = math::FloatRect{element.absolutePosition, element.absoluteSize};
    spriteRenderer.drawRect(bb, color, borderWidth);
}
