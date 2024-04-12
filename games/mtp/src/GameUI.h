#pragma once

#include <string>
#include <vector>

#include <edbr/Graphics/Bouncer.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/Sprite.h>
#include <edbr/Math/Transform.h>

#include <edbr/UI/Element.h>

#include <edbr/DevTools/UIInspector.h>

#include <edbr/GameUI/DialogueBox.h>

#include "Components.h"

class SpriteRenderer;
class Camera;
class AudioManager;

class GameUI {
public:
    struct UIContext {
        const Camera& camera;
        glm::ivec2 screenSize;
        glm::vec3 playerPos;
        InteractComponent::Type interactionType{InteractComponent::Type::None};
    };

public:
    void init(GfxDevice& gfxDevice, AudioManager& audioManager);
    void handleMouseInput(const glm::vec2& mousePos, bool leftMousePressed);
    void update(float dt);
    void updateDevTools(float dt);

    void draw(SpriteRenderer& spriteRenderer, const UIContext& ctx) const;

    DialogueBox& getDialogueBox() { return dialogueBox; }

private:
    void createTestUI(GfxDevice& gfxDevice);
    void drawInteractTip(SpriteRenderer& uiRenderer, const UIContext& ctx) const;

    Font defaultFont;

    std::vector<std::string> strings;

    Sprite interactTipSprite;
    Sprite talkTipSprite;
    Bouncer interactTipBouncer;

    std::unique_ptr<ui::Element> testUI;
    DialogueBox dialogueBox;

    UIInspector uiInspector;
    bool drawBlackBG{false};
};
