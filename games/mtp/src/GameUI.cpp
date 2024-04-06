#include "GameUI.h"

#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/Camera.h>
#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/SpriteRenderer.h>

#include <utf8.h>

#include <edbr/Util/ImGuiUtil.h>
#include <misc/cpp/imgui_stdlib.h>

#include <edbr/UI/ListLayoutElement.h>
#include <edbr/UI/PaddingElement.h>
#include <edbr/UI/TextButton.h>

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

    ui::NineSliceStyle nsStyle;

    JsonFile file(std::filesystem::path{"assets/ui/nine_slice.json"});
    nsStyle.load(file.getLoader(), gfxDevice);

    rootUIElement = std::make_unique<ui::NineSliceElement>(nsStyle, glm::vec2{640.f, 256.f});
    rootUIElement->setPosition(glm::vec2{64.f, 64.f});
    rootUIElement->setAutomaticSizing(ui::Element::AutomaticSizing::XY);

    auto settingsValueColumn =
        std::make_unique<ui::ListLayoutElement>(ui::ListLayoutElement::Direction::Vertical);

    auto settingsNameColumn =
        std::make_unique<ui::ListLayoutElement>(ui::ListLayoutElement::Direction::Vertical);
    const auto strings = std::vector<
        std::string>{"Sound", "Subtitles", "Music volume", "SFX volume", "Speech volume", "Device"};
    for (const auto& str : strings) {
        auto textLabel = std::make_unique<ui::TextLabel>(str, defaultFont);
        textLabel->setPadding({24.f, 8.f, 0.f, 0.f});
        settingsNameColumn->addChild(std::move(textLabel));

        auto valueLabel = std::make_unique<ui::TextLabel>("<setting>", defaultFont);
        valueLabel->setPadding({32.f, 32.f, 0.f, 0.f});
        settingsValueColumn->addChild(std::move(valueLabel));
    }
    settingsNameColumn->applyLayout();
    settingsValueColumn->applyLayout();

    auto buttonsRow =
        std::make_unique<ui::ListLayoutElement>(ui::ListLayoutElement::Direction::Horizontal);
    buttonsRow->addChild(std::make_unique<ui::TextButton>(nsStyle, defaultFont, "OK"));
    buttonsRow->addChild(std::make_unique<ui::TextButton>(nsStyle, defaultFont, "Cancel"));
    buttonsRow->applyLayout();

    auto settingsMainLayout =
        std::make_unique<ui::ListLayoutElement>(ui::ListLayoutElement::Direction::Horizontal);
    settingsMainLayout->addChild(std::move(settingsNameColumn));
    settingsMainLayout->addChild(std::move(settingsValueColumn));
    settingsMainLayout->applyLayout();

    auto settingsLayout =
        std::make_unique<ui::ListLayoutElement>(ui::ListLayoutElement::Direction::Vertical);
    settingsLayout->addChild(std::move(settingsMainLayout));
    settingsLayout->addChild(std::make_unique<ui::PaddingElement>(glm::vec2{0.f, 32.f}));
    settingsLayout->addChild(std::move(buttonsRow));
    settingsLayout->addChild(std::make_unique<ui::PaddingElement>(glm::vec2{0.f, 16.f}));
    settingsLayout->applyLayout();

    rootUIElement->addChild(std::move(settingsLayout));
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
    auto pos = rootUIElement->getPosition();
    if (ImGui::Drag("UI pos", &pos)) {
        rootUIElement->setPosition(pos);
    }

    if (ImGui::InputTextMultiline("text", &strings[0])) {
        auto& textLabel = static_cast<ui::TextLabel&>(*rootUIElement->getChildren()[0]);
        textLabel.setText(strings[0]);
    }
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
        uiRenderer.drawText(tl->getFont(), tl->getText(), textPos, LinearColor::White());

        auto pos = parentPos + tl->getPosition();
        auto cs = tl->getContentSize();
        // uiRenderer.drawRect({pos, cs}, LinearColor{1.f, 0.f, 0.f, 1.f});
    }

    for (const auto& childPtr : element.getChildren()) {
        drawUIElement(uiRenderer, *childPtr, parentPos + element.getPosition());
    }
}
