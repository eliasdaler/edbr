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
    GameUI();
    void init(GfxDevice& gfxDevice, IAudioManager& audioManager);

    bool capturesInput() const;
    void handleInput(const ActionMapping& am);
    void update(const glm::vec2 screenSize, float dt);
    void draw(GfxDevice& gfxDevice, SpriteRenderer& spriteRenderer, const UIContext& ctx);

    DialogueBox& getDialogueBox() override { return dialogueBox; }
    bool isDialogueBoxOpen() const override;
    void openDialogueBox() override;
    void closeDialogueBox() override;

private:
    void drawInteractTip(GfxDevice& gfxDevice, SpriteRenderer& spriteRenderer, const UIContext& ctx)
        const;

    DialogueBox dialogueBox;
    Font defaultFont;
    Cursor cursor;
    MenuStack menuStack;

    // DI
    IAudioManager* audioManager;

    Sprite examineTipSprite;
    Sprite talkTipSprite;
    Sprite goInsideTipSprite;
    Bouncer interactTipBouncer;
};
