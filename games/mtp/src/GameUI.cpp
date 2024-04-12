#include "GameUI.h"

#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/Camera.h>
#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/SpriteRenderer.h>

#include <utf8.h>

#include <edbr/Util/ImGuiUtil.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <edbr/UI/ImageElement.h>
#include <edbr/UI/ListLayoutElement.h>
#include <edbr/UI/NineSliceElement.h>
#include <edbr/UI/RectElement.h>
#include <edbr/UI/TextElement.h>
#include <edbr/UI/UIRenderer.h>

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

void GameUI::init(GfxDevice& gfxDevice, AudioManager& audioManager)
{
    strings = {
        "テキストのレンダリング、すごい",
        "Text rendering works!",
        "Multiline text\nworks too",
    };

    auto neededCodePoints = getUsedCodePoints(strings, true);

    // bool ok = defaultFont.load(gfxDevice, "assets/fonts/DejaVuSerif.ttf", 20, neededCodePoints);
    bool ok = defaultFont
                  .load(gfxDevice, "assets/fonts/JF-Dot-Kappa20B.ttf", 40, neededCodePoints, false);

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

    createTestUI(gfxDevice);

    dialogueBox.init(gfxDevice, audioManager);
    dialogueBox.setVisible(false);
}

void GameUI::handleMouseInput(const glm::vec2& mousePos, bool leftMousePressed)
{}

void GameUI::update(float dt)
{
    interactTipBouncer.update(dt);

    // const auto screenSize = glm::vec2{640, 480};
    const auto screenSize = glm::vec2{1440, 1080};
    testUI->calculateLayout(screenSize);
    dialogueBox.update(screenSize, dt);
}

void GameUI::draw(SpriteRenderer& spriteRenderer, const UIContext& ctx) const
{
    if (!dialogueBox.isVisible()) {
        if (ctx.interactionType != InteractComponent::Type::None) {
            drawInteractTip(spriteRenderer, ctx);
        }
    }

    if (drawBlackBG) {
        spriteRenderer.drawFilledRect({{}, {640, 480}}, LinearColor::Black());
    }
    // ui::drawElement(spriteRenderer, *testUI);

    dialogueBox.draw(spriteRenderer);

    if (uiInspector.drawUIElementBoundingBoxes) {
        uiInspector.drawBoundingBoxes(spriteRenderer, *testUI);
    }
    if (uiInspector.hasSelectedElement()) {
        uiInspector.drawSelectedElement(spriteRenderer);
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
    if (ImGui::Button("Deselect element")) {
        uiInspector.deselectSelectedElement();
    }
    ImGui::Checkbox("Draw black BG", &drawBlackBG);
    ImGui::Checkbox("Draw bounding boxes", &uiInspector.drawUIElementBoundingBoxes);
    ImGui::Separator();
    uiInspector.showUITree(*testUI);

    if (uiInspector.hasSelectedElement()) {
        if (ImGui::Begin("Selected UI element")) {
            uiInspector.showSelectedElementInfo();
        }
        ImGui::End();
    }
}

void GameUI::createTestUI(GfxDevice& gfxDevice)
{
    ui::NineSliceStyle nsStyle;

    JsonFile file(std::filesystem::path{"assets/ui/nine_slice.json"});
    nsStyle.load(file.getLoader(), gfxDevice);

    auto listElement = std::make_unique<ui::ListLayoutElement>();
    listElement->direction = ui::ListLayoutElement::Direction::Horizontal;
    listElement->direction = ui::ListLayoutElement::Direction::Vertical;
    listElement->autoSize = true;
    listElement->offsetPosition = {32.f, 16.f};
    listElement->offsetSize = {-64.f, -32.f};
    // listElement->relativePosition = {0.5f, 0.5f};
    // listElement->origin = {0.5f, 0.5f};
    listElement->padding = 4.f;
    // listElement->autoSizeChildren = false;

    struct ButtonInfo {
        std::string text;
        LinearColor color;
    };
    const auto buttons = std::vector<ButtonInfo>{
        {"AAA", LinearColor{1.f, 0.f, 0.f}},
        {"Hello, world", LinearColor{1.f, 1.f, 0.f}},
        {"Yep...", LinearColor{1.f, 0.f, 1.f}},
        {"Test!", LinearColor{0.f, 1.f, 1.f}},
    };

    for (const auto& bi : buttons) {
        auto textElement = std::make_unique<ui::TextElement>(bi.text, defaultFont, bi.color);

        // text is centered inside the button
        textElement->origin = {0.5f, 0.5f};
        textElement->relativePosition = {0.5f, 0.5f};
        // (16px, 8px) padding is added on both sides
        textElement->offsetSize = {-32.f, -16.f};

        auto nsElement = std::make_unique<ui::NineSliceElement>(nsStyle);
        nsElement->autoSize = true;

        nsElement->addChild(std::move(textElement));
        listElement->addChild(std::move(nsElement));
    }

    auto nsElement = std::make_unique<ui::NineSliceElement>(nsStyle);
    testUI = std::move(nsElement);

    testUI->relativePosition = {0.5f, 0.5f};
    testUI->origin = {0.5f, 0.5f};
    testUI->autoSize = true;
    // rootUIElement->fixedSize = {300.f, 300.f};

    testUI->addChild(std::move(listElement));

    // menu name
    auto menuName =
        std::make_unique<ui::TextElement>("Menu", defaultFont, LinearColor::FromRGB(254, 214, 124));
    const auto fontHeight = defaultFont.lineSpacing;
    menuName->offsetPosition = {std::round(fontHeight / 2.f), -fontHeight};
    menuName->shadow = true;
    testUI->addChild(std::move(menuName));
}
