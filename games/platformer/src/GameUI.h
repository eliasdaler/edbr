#pragma once

#include <edbr/GameUI/Cursor.h>
#include <edbr/GameUI/DialogueBox.h>
#include <edbr/GameUI/IGameUI.h>
#include <edbr/GameUI/MenuStack.h>

#include "Components.h"

class AudioManager;
class ActionMapping;

class GameUI : public IGameUI {
public:
    struct UIContext {
        glm::vec2 playerPos;
        InteractComponent::Type interactionType{InteractComponent::Type::None};
        glm::vec2 cameraPos;
    };

public:
    GameUI(AudioManager& audioManager);
    void init(GfxDevice& gfxDevice);

    bool capturesInput() const;
    void handleInput(const ActionMapping& am);
    void update(const glm::vec2 screenSize, float dt);
    void draw(SpriteRenderer& spriteRenderer, const UIContext& ctx);

    DialogueBox& getDialogueBox() override { return dialogueBox; }
    bool isDialogueBoxOpen() const override;
    void openDialogueBox() override;
    void closeDialogueBox() override;

private:
    void drawInteractTip(SpriteRenderer& spriteRenderer, const UIContext& ctx) const;

    DialogueBox dialogueBox;
    Font defaultFont;
    Cursor cursor;
    MenuStack menuStack;
    AudioManager& audioManager;

    Sprite examineTipSprite;
    Sprite talkTipSprite;
    Sprite goInsideTipSprite;
    Bouncer interactTipBouncer;
};
