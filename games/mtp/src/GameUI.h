#pragma once

#include <string>
#include <vector>

#include <edbr/Graphics/Bouncer.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/Sprite.h>
#include <edbr/Math/Transform.h>

#include <edbr/UI/Element.h>

#include <edbr/DevTools/UIInspector.h>

#include <edbr/GameUI/Cursor.h>
#include <edbr/GameUI/DialogueBox.h>
#include <edbr/GameUI/IGameUI.h>
#include <edbr/GameUI/MenuStack.h>

#include "Components.h"

class SpriteRenderer;
class Camera;
class IAudioManager;
class ActionListManager;
class Action;
class InputManager;
class ActionList;

namespace ui
{
class ButtonElement;
struct ListLayoutElement;
struct NineSliceStyle;
struct ButtonStyle;
}

class Game;

class GameUI : public IGameUI {
public:
    struct UIContext {
        const Camera& camera;
        glm::ivec2 screenSize;
        glm::vec3 playerPos;
        InteractComponent::Type interactionType{InteractComponent::Type::None};
    };

public:
    GameUI(ActionListManager& am);

    void init(
        Game& game,
        GfxDevice& gfxDevice,
        const glm::ivec2& screenSize,
        IAudioManager& audioManager);
    bool capturesInput() const;
    void handleInput(const InputManager& inputManager, float dt);
    void update(float dt);
    void updateDevTools(float dt);

    void onScreenSizeChanged(const glm::ivec2 newScreenSize);

    void draw(SpriteRenderer& spriteRenderer, const UIContext& ctx) const;

    DialogueBox& getDialogueBox() override { return dialogueBox; }
    void openDialogueBox() override;
    void closeDialogueBox() override;
    bool isDialogueBoxOpen() const override;

    void doFadeInFromBlack(float duration);
    void doFadeOutToBlack(float duration);

    std::unique_ptr<Action> fadeInFromBlackAction(float duration);
    std::unique_ptr<Action> fadeOutToBlackAction(float duration);

    void enterMainMenu();
    void exitMainMenu();
    bool isInMainMenu() const;

    void enterPauseMenu();
    void closePauseMenu();
    bool isInPauseMenu() const;

    const Font& getDefaultFont() const { return defaultFont; }

private:
    void createTitleScreen(Game& game, GfxDevice& gfxDevice);
    void createMenus(Game& game, GfxDevice& gfxDevice);
    void createMainMenu(Game& game, GfxDevice& gfxDevice, const ui::NineSliceStyle& nsStyle);
    void createSettingsMenu(Game& game, GfxDevice& gfxDevice, const ui::NineSliceStyle& nsStyle);
    void createPauseMenu(Game& game, GfxDevice& gfxDevice, const ui::NineSliceStyle& nsStyle);

    void drawInteractTip(SpriteRenderer& uiRenderer, const UIContext& ctx) const;

    void enterMenu(ui::Element& menu);
    void exitMenu(ui::Element& menu);

    std::unique_ptr<ui::ButtonElement> makeSimpleButton(
        const ui::ButtonStyle& buttonStyle,
        const std::string& text,
        std::function<void()> onPressed);

    struct ButtonDesc {
        std::string text;
        std::function<void()> onPressed;
        std::string buttonTag;
    };
    std::unique_ptr<ui::ListLayoutElement> makeSimpleButtonList(
        const ui::NineSliceStyle& nsStyle,
        const std::vector<ButtonDesc>& buttons);

    Cursor cursor;
    MenuStack menuStack;

    Font defaultFont;
    Font titleFont;

    Sprite interactTipSprite;
    Sprite talkTipSprite;
    Sprite saveTipSprite;
    Bouncer interactTipBouncer;

    std::unique_ptr<ui::Element> mainMenu;
    std::unique_ptr<ui::Element> titleScreenUI;
    std::string mainMenuTag{"main_menu"};

    std::unique_ptr<ui::Element> settingsMenu;

    std::unique_ptr<ui::Element> pauseMenu;
    std::string pauseMenuTag{"pause_menu"};

    DialogueBox dialogueBox;

    UIInspector uiInspector;

    glm::vec2 screenSize;
    float fadeLevel{0.f};

    // DI
    ActionListManager& actionListManager;
    IAudioManager* audioManager{nullptr};
};
