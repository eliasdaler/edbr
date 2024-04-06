#include "GameUI.h"

#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/Camera.h>
#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/SpriteRenderer.h>

#include <utf8.h>

#include <edbr/Util/ImGuiUtil.h>

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

    auto textLabel = std::make_unique<ui::TextLabel>(strings[0], defaultFont);
    const auto innerPadding = glm::vec2{16.f, 0.f};
    const auto textHeight = 32.f;
    textLabel->setPosition(glm::vec2{0.f, textHeight} + innerPadding);

    rootUIElement->addChild(std::move(textLabel));
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
}

void GameUI::drawUIElement(
    SpriteRenderer& uiRenderer,
    const ui::Element& element,
    const glm::vec2& parentPos) const
{
    if (auto ns = dynamic_cast<const ui::NineSliceElement*>(&element); ns) {
        ns->getNineSlice().draw(uiRenderer, element.getPosition(), ns->getSize());
    }
    if (auto tl = dynamic_cast<const ui::TextLabel*>(&element); tl) {
        uiRenderer.drawText(
            tl->getFont(), tl->getText(), parentPos + element.getPosition(), LinearColor::White());

        auto bb = tl->getFont().calculateTextBoundingBox(tl->getText());
        bb.left += parentPos.x + element.getPosition().x;
        bb.top += parentPos.y + element.getPosition().y;
        uiRenderer.drawRect(bb, LinearColor{1.f, 0.f, 0.f, 1.f});
    }

    for (const auto& childPtr : element.getChildren()) {
        drawUIElement(uiRenderer, *childPtr, parentPos + element.getPosition());
    }
}
