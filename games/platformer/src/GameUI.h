#pragma once

#include <edbr/GameUI/Cursor.h>
#include <edbr/GameUI/DialogueBox.h>
#include <edbr/GameUI/IGameUI.h>
#include <edbr/GameUI/MenuStack.h>

class AudioManager;
class ActionMapping;

class GameUI : public IGameUI {
public:
    GameUI(AudioManager& audioManager);
    void init(GfxDevice& gfxDevice);

    bool capturesInput() const;
    void handleInput(const ActionMapping& am);
    void update(const glm::vec2 screenSize, float dt);
    void draw(SpriteRenderer& spriteRenderer);

    DialogueBox& getDialogueBox() override { return dialogueBox; }
    bool isDialogueBoxOpen() const override;
    void openDialogueBox() override;
    void closeDialogueBox() override;

private:
    DialogueBox dialogueBox;
    Font defaultFont;
    Cursor cursor;
    MenuStack menuStack;
    AudioManager& audioManager;
};
