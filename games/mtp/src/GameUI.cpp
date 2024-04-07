#include "GameUI.h"

#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/Camera.h>
#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/SpriteRenderer.h>

#include <utf8.h>

#include <edbr/DevTools/ImGuiPropertyTable.h>
#include <edbr/Util/ImGuiUtil.h>
#include <edbr/Util/Palette.h>
#include <misc/cpp/imgui_stdlib.h>

#include <edbr/UI/ListLayoutElement.h>
#include <edbr/UI/PaddingElement.h>
#include <edbr/UI/TextButton.h>

#ifdef __GNUG__
#include <cstdlib>
#include <cxxabi.h>
#include <memory>
#endif

namespace
{

std::unordered_set<std::uint32_t> getUsedCodePoints(
    const std::vector<std::string>& strings,
    bool includeAllASCII)
{
    std::unordered_set<std::uint32_t> cps;
    if (includeAllASCII) {
        for (std::uint32_t i = 0; i < 255; ++i) {
            cps.insert(i);
        }
    }
    for (const auto& s : strings) {
        auto it = s.begin();
        const auto e = s.end();
        while (it != e) {
            cps.insert(utf8::next(it, e));
        }
    }
    return cps;
}
}

void GameUI::init(GfxDevice& gfxDevice)
{
    strings = {
        "テキストのレンダリング、すごい",
        "Text rendering works!",
        "Multiline text\nworks too",
    };

    auto neededCodePoints = getUsedCodePoints(strings, true);

    bool ok = defaultFont.load(gfxDevice, "assets/fonts/JF-Dot-Kappa20B.ttf", 32, neededCodePoints);

    assert(ok && "font failed to load");

    const auto interactTipImageId =
        gfxDevice.loadImageFromFile("assets/images/ui/interact_tip.png");
    const auto& interactTipImage = gfxDevice.getImage(interactTipImageId);
    interactTipSprite.setTexture(interactTipImage);
    interactTipSprite.setPivotPixel({26, 60});

    const auto talkTipImageId = gfxDevice.loadImageFromFile("assets/images/ui/talk_tip.png");
    const auto& talkTipImage = gfxDevice.getImage(talkTipImageId);
    talkTipSprite.setTexture(talkTipImage);
    talkTipSprite.setPivotPixel({72, 72});

    interactTipBouncer = Bouncer({
        .maxOffset = 4.f,
        .moveDuration = 0.5f,
        .tween = glm::quadraticEaseInOut<float>,
    });

    createOptionsMenu(gfxDevice);
}

void GameUI::update(float dt)
{
    interactTipBouncer.update(dt);
}

void GameUI::draw(SpriteRenderer& uiRenderer, const UIContext& ctx) const
{
    if (ctx.interactionType != InteractComponent::Type::None) {
        drawInteractTip(uiRenderer, ctx);
    }

    drawUIElement(uiRenderer, *rootUIElement, {});
    if (drawUIElementBoundingBoxes) {
        drawUIBoundingBoxes(uiRenderer, *rootUIElement, {});
    }
}

void GameUI::drawInteractTip(SpriteRenderer& uiRenderer, const UIContext& ctx) const
{
    const Sprite& sprite = [this](InteractComponent::Type type) {
        switch (type) {
        case InteractComponent::Type::Interact:
            return interactTipSprite;
        case InteractComponent::Type::Talk:
            return talkTipSprite;
        default:
            return interactTipSprite;
        }
    }(ctx.interactionType);

    auto screenPos = edbr::util::fromWorldCoordsToScreenCoords(
        ctx.playerPos + glm::vec3{0.f, 1.7f, 0.f}, ctx.camera.getViewProj(), ctx.screenSize);
    screenPos.y += interactTipBouncer.getOffset();
    uiRenderer.drawSprite(sprite, screenPos);
}

void GameUI::updateDevTools(float dt)
{
    ImGui::Checkbox("Draw BB", &drawUIElementBoundingBoxes);
    auto pos = rootUIElement->getPosition();
    if (ImGui::Drag("UI pos", &pos)) {
        rootUIElement->setPosition(pos);
    }

    updateUITree(*rootUIElement);
    if (selectedUIElement) {
        if (ImGui::Begin("Selected UI element")) {
            updateSelectedUIElementInfo(*selectedUIElement);
        }
        ImGui::End();
    }
}

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

void GameUI::updateUITree(const ui::Element& element)
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
            updateUITree(*child);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void GameUI::updateSelectedUIElementInfo(const ui::Element& element)
{
    using namespace devtools;

    BeginPropertyTable();

    DisplayProperty("Position", element.getPosition());
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

void GameUI::drawUIElement(
    SpriteRenderer& uiRenderer,
    const ui::Element& element,
    const glm::vec2& parentPos) const
{
    if (auto ns = dynamic_cast<const ui::NineSliceElement*>(&element); ns) {
        ns->getNineSlice().draw(uiRenderer, parentPos + element.getPosition(), ns->getSize());
    }

    if (auto tl = dynamic_cast<const ui::TextLabel*>(&element); tl) {
        auto p = tl->getPadding();
        auto textPos = parentPos + element.getPosition() + glm::vec2{p.left, p.top} +
                       glm::vec2{0.f, tl->getFont().lineSpacing};
        uiRenderer.drawText(tl->getFont(), tl->getText(), textPos, tl->getColor());
    }

    for (const auto& childPtr : element.getChildren()) {
        drawUIElement(uiRenderer, *childPtr, parentPos + element.getPosition());
    }
}

void GameUI::drawUIBoundingBoxes(
    SpriteRenderer& uiRenderer,
    const ui::Element& element,
    const glm::vec2& parentPos) const
{
    const auto pos = parentPos + element.getPosition();
    const auto cs = element.getSize();
    auto bbColor =
        edbr::rgbToLinear(util::pickRandomColor(util::LightColorsPalette, (void*)&element));
    uiRenderer.drawRect({pos, cs}, bbColor);
    bbColor.a = 0.25f;
    uiRenderer.drawFilledRect({pos, cs}, bbColor);

    if (&element == selectedUIElement) {
        static int frameNum = 0.f;
        ++frameNum;
        float v = std::abs(std::sin((float)frameNum / 60.f));
        bbColor.r *= v;
        bbColor.g *= v;
        bbColor.b *= v;
        bbColor.a = 0.5f;
        uiRenderer.drawFilledRect({pos, cs}, bbColor);
    }

    for (const auto& childPtr : element.getChildren()) {
        drawUIBoundingBoxes(uiRenderer, *childPtr, parentPos + element.getPosition());
    }
}

void GameUI::createOptionsMenu(GfxDevice& gfxDevice)
{
    ui::NineSliceStyle nsStyle;

    JsonFile file(std::filesystem::path{"assets/ui/nine_slice.json"});
    nsStyle.load(file.getLoader(), gfxDevice);

    rootUIElement = std::make_unique<ui::NineSliceElement>(nsStyle, glm::vec2{640.f, 256.f});
    rootUIElement->setPosition(glm::vec2{64.f, 64.f});
    rootUIElement->setAutomaticSizing(ui::Element::AutomaticSizing::XY);

    auto settingsNameColumn =
        std::make_unique<ui::ListLayoutElement>(ui::ListLayoutElement::Direction::Vertical);
    auto settingsValueColumn =
        std::make_unique<ui::ListLayoutElement>(ui::ListLayoutElement::Direction::Vertical);
    const auto strings = std::vector<std::string>{
        "Sound",
        "Subtitles",
        "Music volume",
        "SFX volume",
        "Speech volume",
        "Device",
    };
    for (const auto& str : strings) {
        auto textLabel = std::make_unique<ui::TextLabel>(str, defaultFont, LinearColor::White());
        textLabel->setPadding({24.f, 8.f, 0.f, 0.f});
        settingsNameColumn->addChild(std::move(textLabel));

        auto valueLabel =
            std::make_unique<ui::TextLabel>("<<<<<setting>>>>>", defaultFont, LinearColor::White());
        valueLabel->setPadding({32.f, 32.f, 0.f, 0.f});
        settingsValueColumn->addChild(std::move(valueLabel));
    }
    settingsNameColumn->applyLayout();
    settingsValueColumn->applyLayout();

    // [setting] [value]
    auto settingsMainLayout =
        std::make_unique<ui::ListLayoutElement>(ui::ListLayoutElement::Direction::Horizontal);
    settingsMainLayout->addChild(std::move(settingsNameColumn));
    settingsMainLayout->addChild(std::move(settingsValueColumn));
    settingsMainLayout->applyLayout();

    // buttons
    auto okButton =
        std::make_unique<ui::TextButton>(nsStyle, defaultFont, "OK", LinearColor::White());
    okButton->setTag("OKButton");

    auto cancelButton =
        std::make_unique<ui::TextButton>(nsStyle, defaultFont, "Cancel", LinearColor::White());
    cancelButton->setTag("CancelButton");

    auto buttonsRow =
        std::make_unique<ui::ListLayoutElement>(ui::ListLayoutElement::Direction::Horizontal);
    buttonsRow->addChild(std::move(okButton));
    buttonsRow->addChild(std::move(cancelButton));
    buttonsRow->setCenteredPositioning(true);
    buttonsRow->applyLayoutCentered(settingsMainLayout->getSize().x);

    // main layout
    // [setting] [value]
    // [setting] [value]
    // [setting] [value]
    // ...
    // [padding]
    // [buttons]
    // [padding]
    auto settingsLayout =
        std::make_unique<ui::ListLayoutElement>(ui::ListLayoutElement::Direction::Vertical);
    settingsLayout->addChild(std::move(settingsMainLayout));
    settingsLayout->addChild(std::make_unique<ui::PaddingElement>(glm::vec2{0.f, 24.f}));
    settingsLayout->addChild(std::move(buttonsRow));
    settingsLayout->addChild(std::make_unique<ui::PaddingElement>(glm::vec2{0.f, 16.f}));
    settingsLayout->applyLayout();

    const auto menuNameLabelColor = LinearColor::FromRGB(254, 214, 124);
    auto menuNameLabel =
        std::make_unique<ui::TextLabel>("Sound Settings", defaultFont, menuNameLabelColor);
    menuNameLabel->setTag("MenuName");
    menuNameLabel->setPosition({16.f, -defaultFont.lineSpacing - 8.f});

    rootUIElement->addChild(std::move(menuNameLabel));
    rootUIElement->addChild(std::move(settingsLayout));
    rootUIElement->setAutomaticSizingElementIndex(1);
}
