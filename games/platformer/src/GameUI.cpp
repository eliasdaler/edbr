#include "GameUI.h"

#include <edbr/Audio/AudioManager.h>
#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/GfxDevice.h>

GameUI::GameUI(AudioManager& audioManager) : menuStack(cursor), audioManager(audioManager)
{}

void GameUI::init(GfxDevice& gfxDevice)
{
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
}

bool GameUI::capturesInput() const
{
    return menuStack.hasMenus();
}

void GameUI::handleInput(const ActionMapping& am)
{
    if (cursor.visible) {
        cursor.handleInput(am, audioManager);
    }

    if (isDialogueBoxOpen()) {
        dialogueBox.handleInput(am);
    }
}

void GameUI::update(const glm::vec2 screenSize, float dt)
{
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
    audioManager.playSound("assets/sounds/ui/menu_close.wav");
}

void GameUI::draw(SpriteRenderer& spriteRenderer)
{
    menuStack.draw(spriteRenderer);
    if (cursor.visible) {
        cursor.draw(spriteRenderer);
    }
}
