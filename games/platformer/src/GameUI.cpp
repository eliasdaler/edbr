#include "GameUI.h"

#include <edbr/Audio/IAudioManager.h>
#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/SpriteRenderer.h>

#include "Components.h"

GameUI::GameUI() : menuStack(cursor)
{}

void GameUI::init(GfxDevice& gfxDevice, IAudioManager& audioManager)
{
    this->audioManager = &audioManager;

    { // dialogue box
        std::filesystem::path dbStylePath{"assets/ui/dialogue_box.json"};
        JsonFile dbStyleFile(dbStylePath);
        assert(dbStyleFile.isGood());

        DialogueBoxStyle dbStyle;
        dbStyle.load(dbStyleFile.getLoader(), gfxDevice);

        dialogueBox.init(dbStyle, gfxDevice, audioManager);

        const auto cursorImageId = gfxDevice.loadImageFromFile("assets/images/ui/hand_cursor.png");
        const auto& cursorImage = gfxDevice.getImage(cursorImageId);
        cursor.sprite.setTexture(cursorImage);
        cursor.sprite.setPivotPixel({15, 7});

        cursor.bouncer = Bouncer({
            .maxOffset = 2.f,
            .moveDuration = 0.5f,
            .tween = glm::quadraticEaseInOut<float>,
        });

        cursor.moveSound = "assets/sounds/ui/cursor_move.wav";
    }

    { // interact sprites
        const auto examineTipImageId =
            gfxDevice.loadImageFromFile("assets/images/ui/check_tip.png");
        const auto& examineTipImage = gfxDevice.getImage(examineTipImageId);
        examineTipSprite.setTexture(examineTipImage);
        examineTipSprite.setPivotPixel({5, 15});

        const auto goInsideTipImageId =
            gfxDevice.loadImageFromFile("assets/images/ui/go_inside_tip.png");
        const auto& goInsideTipImage = gfxDevice.getImage(goInsideTipImageId);
        goInsideTipSprite.setTexture(goInsideTipImage);
        goInsideTipSprite.setPivotPixel({7, 13});

        const auto talkTipImageId = gfxDevice.loadImageFromFile("assets/images/ui/talk_tip.png");
        const auto& talkTipImage = gfxDevice.getImage(talkTipImageId);
        talkTipSprite.setTexture(talkTipImage);
        talkTipSprite.setPivotPixel({5, 15});

        interactTipBouncer = Bouncer({
            .maxOffset = 2.f,
            .moveDuration = 0.5f,
            .tween = glm::quadraticEaseInOut<float>,
        });
    }
}

bool GameUI::capturesInput() const
{
    return menuStack.hasMenus();
}

void GameUI::handleInput(const ActionMapping& am)
{
    assert(audioManager && "init not called");

    if (cursor.visible) {
        cursor.handleInput(am, *audioManager);
    }

    if (isDialogueBoxOpen()) {
        dialogueBox.handleInput(am);
    }
}

void GameUI::update(const glm::vec2 screenSize, float dt)
{
    interactTipBouncer.update(dt);
    cursor.update(dt);
    dialogueBox.update(dt);
    menuStack.calculateLayout(screenSize);
}

bool GameUI::isDialogueBoxOpen() const
{
    return menuStack.isInsideMenu(DialogueBox::DialogueBoxMenuTag);
}

void GameUI::openDialogueBox()
{
    if (isDialogueBoxOpen()) {
        return;
    }
    menuStack.pushMenu(dialogueBox.getRootElement());
}

void GameUI::closeDialogueBox()
{
    assert(isDialogueBoxOpen());
    menuStack.popMenu();
    dialogueBox.resetState();
    audioManager->playSound("assets/sounds/ui/menu_close.wav");
}

void GameUI::draw(GfxDevice& gfxDevice, SpriteRenderer& spriteRenderer, const UIContext& ctx)
{
    menuStack.draw(gfxDevice, spriteRenderer);
    if (cursor.visible) {
        cursor.draw(gfxDevice, spriteRenderer);
    }

    if (!isDialogueBoxOpen() && ctx.interactionType != InteractComponent::Type::None) {
        drawInteractTip(gfxDevice, spriteRenderer, ctx);
    }
}

void GameUI::drawInteractTip(
    GfxDevice& gfxDevice,
    SpriteRenderer& spriteRenderer,
    const UIContext& ctx) const
{
    const Sprite& sprite = [this](InteractComponent::Type type) {
        switch (type) {
        case InteractComponent::Type::Examine:
            return examineTipSprite;
        case InteractComponent::Type::Talk:
            return talkTipSprite;
        case InteractComponent::Type::GoInside:
            return goInsideTipSprite;
        default:
            return examineTipSprite;
        }
    }(ctx.interactionType);

    auto screenPos = ctx.playerPos + glm::vec2{0.f, -16.f} - ctx.cameraPos;
    screenPos.y += interactTipBouncer.getOffset();
    spriteRenderer.drawSprite(gfxDevice, sprite, screenPos);
}
