#include "GameUI.h"

#include <edbr/Audio/AudioManager.h>
#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/Camera.h>
#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/SpriteRenderer.h>
#include <edbr/Input/InputManager.h>

#include <utf8.h>

#include <edbr/DevTools/ImGuiPropertyTable.h>
#include <edbr/Util/ImGuiUtil.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <edbr/UI/ButtonElement.h>
#include <edbr/UI/ImageElement.h>
#include <edbr/UI/ListLayoutElement.h>
#include <edbr/UI/NineSliceElement.h>
#include <edbr/UI/RectElement.h>
#include <edbr/UI/TextElement.h>
#include <edbr/UI/UIRenderer.h>

#include <edbr/ActionList/ActionListManager.h>
#include <edbr/ActionList/ActionWrappers.h>

#include "Game.h"

GameUI::GameUI(ActionListManager& am, AudioManager& audioManager) :
    menuStack(cursor), actionListManager(am), audioManager(audioManager)
{}

void GameUI::init(Game& game, GfxDevice& gfxDevice, const glm::ivec2& screenSize)
{
    this->onScreenSizeChanged(screenSize);

    // bool ok = defaultFont.load(gfxDevice, "assets/fonts/DejaVuSerif.ttf", 20, neededCodePoints);
    bool ok = defaultFont.load(
        gfxDevice,
        "assets/fonts/JF-Dot-Kappa20B.ttf",
        40,
        game.getTextManager().getUsedCodePoints(),
        false);

    assert(ok && "font failed to load");

    ok = titleFont.load(
        gfxDevice,
        "assets/fonts/LoveLetter.ttf",
        120,
        game.getTextManager().getUsedCodePoints(),
        true);

    assert(ok && "font failed to load");

    const auto interactTipImageId =
        gfxDevice.loadImageFromFile("assets/images/ui/interact_tip.png");
    const auto& interactTipImage = gfxDevice.getImage(interactTipImageId);
    interactTipSprite.setTexture(interactTipImage);
    interactTipSprite.setPivotPixel({26, 60});

    const auto saveTipImageId = gfxDevice.loadImageFromFile("assets/images/ui/save_tip.png");
    const auto& saveTipImage = gfxDevice.getImage(saveTipImageId);
    saveTipSprite.setTexture(saveTipImage);
    saveTipSprite.setPivotPixel({80, 180});

    const auto talkTipImageId = gfxDevice.loadImageFromFile("assets/images/ui/talk_tip.png");
    const auto& talkTipImage = gfxDevice.getImage(talkTipImageId);
    talkTipSprite.setTexture(talkTipImage);
    talkTipSprite.setPivotPixel({72, 72});

    { // cursor
        const auto cursorImageId = gfxDevice.loadImageFromFile("assets/images/ui/hand_cursor.png");
        const auto& cursorImage = gfxDevice.getImage(cursorImageId);
        cursor.sprite.setTexture(cursorImage);
        cursor.sprite.setPivotPixel({15, 7});
        cursor.spriteScale = glm::vec2{3.f};

        cursor.bouncer = Bouncer({
            .maxOffset = 2.f,
            .moveDuration = 0.5f,
            .tween = glm::quadraticEaseInOut<float>,
        });

        cursor.moveSound = "assets/sounds/ui/cursor_move.wav";
        cursor.errorSound = "assets/sounds/ui/menu_error.wav";
    }

    interactTipBouncer = Bouncer({
        .maxOffset = 4.f,
        .moveDuration = 0.5f,
        .tween = glm::quadraticEaseInOut<float>,
    });

    createTitleScreen(game, gfxDevice);
    createMenus(game, gfxDevice);

    { // dialogue box
        std::filesystem::path dbStylePath{"assets/ui/dialogue_box.json"};
        JsonFile dbStyleFile(dbStylePath);
        assert(dbStyleFile.isGood());

        DialogueBoxStyle dbStyle;
        dbStyle.load(dbStyleFile.getLoader(), gfxDevice);

        dialogueBox.init(dbStyle, gfxDevice, audioManager);
    }
}

bool GameUI::isDialogueBoxOpen() const
{
    return menuStack.isInsideMenu(DialogueBox::DialogueBoxMenuTag);
}

void GameUI::onScreenSizeChanged(const glm::ivec2 newScreenSize)
{
    screenSize = static_cast<glm::ivec2>(newScreenSize);
}

bool GameUI::capturesInput() const
{
    return menuStack.hasMenus();
}

void GameUI::handleInput(const InputManager& inputManager, float dt)
{
    const auto& am = inputManager.getActionMapping();
    static const auto menuBackAction = am.getActionTagHash("MenuBack");

    if (cursor.visible) {
        cursor.handleInput(am, audioManager);
    }

    if (menuStack.hasMenus()) {
        if (isDialogueBoxOpen()) {
            dialogueBox.handleInput(inputManager.getActionMapping());
        } else if (!isInPauseMenu()) { // pause menu inputs handled by the game itself
            if (am.wasJustPressed(menuBackAction)) {
                menuStack.menuGoBack();
                audioManager.playSound("assets/sounds/ui/menu_close.wav");
            }
        }
    }
}

void GameUI::update(float dt)
{
    cursor.update(dt);
    interactTipBouncer.update(dt);

    if (isDialogueBoxOpen()) {
        dialogueBox.update(dt);
    }
}

void GameUI::draw(SpriteRenderer& spriteRenderer, const UIContext& ctx) const
{
    // ideally it should be calculated in update, but sometimes
    // it's possible to add new menus to stack betwee update and draw
    menuStack.calculateLayout(screenSize);

    if (isInMainMenu()) {
        titleScreenUI->calculateLayout(screenSize);
        ui::drawElement(spriteRenderer, *titleScreenUI);
    }

    if (!isDialogueBoxOpen()) {
        if (ctx.interactionType != InteractComponent::Type::None) {
            drawInteractTip(spriteRenderer, ctx);
        }
    }

    if (isInPauseMenu()) {
        spriteRenderer.drawFilledRect({{}, screenSize}, LinearColor{0.f, 0.f, 0.f, 0.9f});
    }

    menuStack.draw(spriteRenderer);

    if (cursor.visible) {
        cursor.draw(spriteRenderer);
    }

    if (fadeLevel != 0.f) {
        spriteRenderer.drawFilledRect({{}, screenSize}, LinearColor{0.f, 0.f, 0.f, fadeLevel});
    }

    uiInspector.draw(spriteRenderer);
}

void GameUI::drawInteractTip(SpriteRenderer& spriteRenderer, const UIContext& ctx) const
{
    const Sprite& sprite = [this](InteractComponent::Type type) {
        switch (type) {
        case InteractComponent::Type::Interact:
            return interactTipSprite;
        case InteractComponent::Type::Talk:
            return talkTipSprite;
        case InteractComponent::Type::Save:
            return saveTipSprite;
        default:
            return interactTipSprite;
        }
    }(ctx.interactionType);

    auto screenPos = edbr::util::fromWorldCoordsToScreenCoords(
        ctx.playerPos + glm::vec3{0.f, 1.7f, 0.f}, ctx.camera.getViewProj(), ctx.screenSize);
    screenPos.y += interactTipBouncer.getOffset();
    spriteRenderer.drawSprite(sprite, screenPos);
}

void GameUI::updateDevTools(float dt)
{
    {
        using namespace devtools;
        BeginPropertyTable();
        DisplayProperty("In pause menu", isInPauseMenu());
        DisplayProperty("Dialogue box on top", isDialogueBoxOpen());
        EndPropertyTable();
    }

    if (ImGui::Button("Deselect element")) {
        uiInspector.deselectSelectedElement();
    }
    ImGui::Checkbox("Draw bounding boxes", &uiInspector.drawUIElementBoundingBoxes);
    ImGui::Separator();
    if (menuStack.hasMenus()) {
        uiInspector.setInspectedUI(menuStack.getTopMenu());
        uiInspector.updateUI(dt);
    }

    ImGui::TextUnformatted("Cursor");
    {
        using namespace devtools;
        BeginPropertyTable();
        DisplayProperty("Visible", cursor.visible);
        if (cursor.visible && cursor.selection) {
            DisplayPropertyCustomBegin("Cursor selection");
            ImGui::Text("Hover to focus");
            if (ImGui::IsItemHovered()) {
                uiInspector.setFocusElement(*cursor.selection);
            }
        } else {
            DisplayProperty("Selection", "null");
        }
        EndPropertyTable();
    }

    if (ImGui::CollapsingHeader("Dialogue box")) {
        dialogueBox.displayDebugInfo();
    }

    ImGui::TextUnformatted("Menu history:");
    for (const auto& menu : menuStack.getMenuHistory()) {
        ImGui::TextUnformatted(menu->tag.empty() ? "unnamed menu" : menu->tag.c_str());
    }
}

void GameUI::createTitleScreen(Game& game, GfxDevice& gfxDevice)
{
    titleScreenUI = std::make_unique<ui::Element>();
    titleScreenUI->fixedSize = screenSize;

    auto& tm = game.getTextManager();
    // centered game title
    auto gameTitleText = std::make_unique<ui::TextElement>(
        tm.getString(LST{"TITLE_NAME"}), titleFont, LinearColor::FromRGB(184, 194, 174));
    gameTitleText->relativePosition = {0.5f, 0.175f};
    gameTitleText->origin = {0.5f, 0.5f};
    titleScreenUI->addChild(std::move(gameTitleText));

    // centered "a game by"
    auto gameByText = std::make_unique<ui::TextElement>(
        tm.getString(LST{"TITLE_GAME_BY"}), defaultFont, LinearColor::FromRGB(205, 209, 194));
    gameByText->offsetPosition = {0.f, -24.f};
    gameByText->relativePosition = {0.5f, 1.f};
    gameByText->origin = {0.5f, 1.f};
    titleScreenUI->addChild(std::move(gameByText));

    // version in bottom-right corner
    auto versionText = std::make_unique<ui::TextElement>(
        game.getVersion().toString(), defaultFont, LinearColor::FromRGB(128, 128, 128));
    versionText->relativePosition = {1.f, 1.f};
    versionText->origin = {1.f, 1.f};
    titleScreenUI->addChild(std::move(versionText));
}

void GameUI::createMenus(Game& game, GfxDevice& gfxDevice)
{
    ui::NineSliceStyle nsStyle;

    JsonFile file(std::filesystem::path{"assets/ui/nine_slice.json"});
    nsStyle.load(file.getLoader(), gfxDevice);

    createMainMenu(game, gfxDevice, nsStyle);
    createSettingsMenu(game, gfxDevice, nsStyle);
    createPauseMenu(game, gfxDevice, nsStyle);
}

void GameUI::createMainMenu(Game& game, GfxDevice& gfxDevice, const ui::NineSliceStyle& nsStyle)
{
    const auto& tm = game.getTextManager();

    auto nsElement = std::make_unique<ui::NineSliceElement>(nsStyle);
    mainMenu = std::move(nsElement);

    mainMenu->relativePosition = {0.5f, 0.75f};
    mainMenu->origin = {0.5f, 0.5f};
    mainMenu->autoSize = true;

    static const char* newGameButtonTag = "new_game_button";
    static const char* loadGameButtonTag = "load_game_button";
    static const char* settingsButtonTag = "settings_button";

    const auto buttons = std::vector<ButtonDesc>{
        {
            .text = tm.getString(LST{"MENU_NEW_GAME"}),
            .onPressed =
                [this, &game]() {
                    exitMainMenu();
                    // so that the sound plays, menu closes etc.
                    game.doWithDelay("start new game", 0.5f, [&game] { game.startNewGame(); });
                },
            .buttonTag = newGameButtonTag,
        },
        {
            .text = tm.getString(LST{"MENU_LOAD_GAME"}),
            .onPressed =
                [this, &game]() {
                    exitMainMenu();
                    // so that the sound plays, menu closes etc.
                    game.doWithDelay("start loaded game", 0.5f, [&game] {
                        game.startLoadedGame();
                    });
                },
            .buttonTag = loadGameButtonTag,
        },
        {
            .text = tm.getString(LST{"MENU_SETTINGS"}),
            .onPressed = [this]() { menuStack.replaceTopMenu(*settingsMenu); },
            .buttonTag = settingsButtonTag,
        },
        {
            .text = tm.getString(LST{"MENU_EXIT"}),
            .onPressed =
                [this, &game]() {
                    exitMainMenu();
                    // so that the sound plays, menu closes etc.
                    game.doWithDelay("exit game", 0.5f, [&game] { game.exitGame(); });
                },
        },
    };

    auto listElement = makeSimpleButtonList(nsStyle, buttons);
    mainMenu->addChild(std::move(listElement));

    mainMenu->visible = false;
    mainMenu->tag = mainMenuTag;

    if (game.hasAnySaveFile()) {
        mainMenu->cursorSelectionStartElement =
            &mainMenu->findElement<ui::ButtonElement>(loadGameButtonTag);
    } else {
        mainMenu->cursorSelectionStartElement =
            &mainMenu->findElement<ui::ButtonElement>(newGameButtonTag);
        mainMenu->findElement<ui::ButtonElement>(loadGameButtonTag).setEnabled(false);
    }
    mainMenu->findElement<ui::ButtonElement>(settingsButtonTag).setEnabled(false);
}

void GameUI::createSettingsMenu(Game& game, GfxDevice& gfxDevice, const ui::NineSliceStyle& nsStyle)
{
    const auto& tm = game.getTextManager();

    auto nsElement = std::make_unique<ui::NineSliceElement>(nsStyle);
    settingsMenu = std::move(nsElement);

    settingsMenu->relativePosition = {0.5f, 0.75f};
    settingsMenu->origin = {0.5f, 0.5f};
    settingsMenu->autoSize = true;

    const auto buttons = std::vector<ButtonDesc>{
        {
            .text = tm.getString(LST{"MENU_SETTING_VIDEO"}),
            .onPressed = []() { fmt::println("video"); },
        },
        {
            .text = tm.getString(LST{"MENU_SETTING_AUDIO"}),
            .onPressed = []() { fmt::println("audio"); },
        },
        {
            .text = tm.getString(LST{"MENU_SETTING_GAMEPLAY"}),
            .onPressed = []() { fmt::println("gameplay"); },
        },
        {
            .text = tm.getString(LST{"MENU_BACK"}),
            .onPressed = [this]() { menuStack.menuGoBack(); },
        },
    };

    auto listElement = makeSimpleButtonList(nsStyle, buttons);
    auto& list = settingsMenu->addChild(std::move(listElement));
    settingsMenu->cursorSelectionStartElement = list.children[0].get();

    // menu name
    auto menuName = std::make_unique<
        ui::TextElement>("Settings", defaultFont, LinearColor::FromRGB(254, 214, 124));
    const auto fontHeight = defaultFont.lineSpacing;
    menuName->offsetPosition = {std::round(fontHeight / 2.f), -fontHeight};
    menuName->shadow = true;
    settingsMenu->addChild(std::move(menuName));

    settingsMenu->visible = false;
    settingsMenu->tag = "settings_menu";
}

void GameUI::createPauseMenu(Game& game, GfxDevice& gfxDevice, const ui::NineSliceStyle& nsStyle)
{
    const auto& tm = game.getTextManager();

    auto nsElement = std::make_unique<ui::NineSliceElement>(nsStyle);
    pauseMenu = std::move(nsElement);

    pauseMenu->relativePosition = {0.5f, 0.75f};
    pauseMenu->origin = {0.5f, 0.5f};
    pauseMenu->autoSize = true;

    static const char* resumeButtonTag = "resume_button";
    static const char* settingsButtonTag = "settings_button";
    const auto buttons = std::vector<ButtonDesc>{
        {
            .text = tm.getString(LST{"MENU_RESUME"}),
            .onPressed = [&game]() { game.exitPauseMenu(); },
            .buttonTag = resumeButtonTag,
        },
        {
            .text = tm.getString(LST{"MENU_SETTINGS"}),
            .onPressed = [this]() { menuStack.replaceTopMenu(*settingsMenu); },
            .buttonTag = settingsButtonTag,
        },
        {
            .text = tm.getString(LST{"MENU_EXIT_TO_TITLE"}),
            .onPressed = [&game]() { game.exitToTitle(); },
        },
    };

    auto listElement = makeSimpleButtonList(nsStyle, buttons);
    auto& list = pauseMenu->addChild(std::move(listElement));
    pauseMenu->cursorSelectionStartElement = list.children[0].get();

    // menu name
    auto menuName = std::make_unique<
        ui::TextElement>("Pause", defaultFont, LinearColor::FromRGB(254, 214, 124));
    const auto fontHeight = defaultFont.lineSpacing;
    menuName->offsetPosition = {std::round(fontHeight / 2.f), -fontHeight};
    menuName->shadow = true;
    pauseMenu->addChild(std::move(menuName));
    pauseMenu->tag = pauseMenuTag;

    pauseMenu->visible = false;

    pauseMenu->cursorSelectionStartElement =
        &pauseMenu->findElement<ui::ButtonElement>(resumeButtonTag);
    pauseMenu->findElement<ui::ButtonElement>(settingsButtonTag).setEnabled(false);
}

void GameUI::enterMainMenu()
{
    assert(!menuStack.hasMenus());
    menuStack.pushMenu(*mainMenu);
}

void GameUI::exitMainMenu()
{
    assert(isInMainMenu());
    menuStack.popMenu();
}

bool GameUI::isInMainMenu() const
{
    return menuStack.isInsideMenu(mainMenuTag);
}

void GameUI::enterPauseMenu()
{
    assert(!menuStack.hasMenus());
    menuStack.pushMenu(*pauseMenu);
    audioManager.playSound("assets/sounds/ui/pause.wav");
}

void GameUI::closePauseMenu()
{
    assert(isInPauseMenu());
    menuStack.popMenu();
    audioManager.playSound("assets/sounds/ui/menu_close.wav");
}

bool GameUI::isInPauseMenu() const
{
    return menuStack.isInsideMenu(pauseMenuTag);
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

std::unique_ptr<ui::ButtonElement> GameUI::makeSimpleButton(
    const ui::ButtonStyle& buttonStyle,
    const std::string& text,
    std::function<void()> onPressed)
{
    return std::make_unique<ui::ButtonElement>(buttonStyle, defaultFont, text, [this, onPressed]() {
        audioManager.playSound("assets/sounds/ui/menu_select.wav");
        onPressed();
    });
}

std::unique_ptr<ui::ListLayoutElement> GameUI::makeSimpleButtonList(
    const ui::NineSliceStyle& nsStyle,
    const std::vector<ButtonDesc>& buttons)
{
    auto listElement = std::make_unique<ui::ListLayoutElement>();
    listElement->direction = ui::ListLayoutElement::Direction::Vertical;
    listElement->autoSize = true;
    listElement->offsetPosition = {32.f, 16.f};
    listElement->offsetSize = {-64.f, -32.f};
    listElement->padding = 4.f;

    const auto buttonPadding = glm::vec2{16.f, 8.f};
    const auto bs = ui::ButtonStyle{
        .nineSliceStyle = &nsStyle,
        .normalColor = LinearColor{1.f, 1.f, 1.f},
        .selectedColor = edbr::rgbToLinear(254, 174, 52),
        .disabledColor = edbr::rgbToLinear(128, 128, 128),
        .padding = buttonPadding,
        .cursorOffset = {0.f, buttonPadding.y + defaultFont.lineSpacing / 2.f},
    };

    for (const auto& button : buttons) {
        auto& child = listElement->addChild(makeSimpleButton(bs, button.text, button.onPressed));
        child.tag = button.buttonTag;
    }

    // handle cursor
    for (std::size_t i = 0; i < listElement->children.size(); ++i) {
        const auto prevIndex = (i == 0) ? listElement->children.size() - 1 : i - 1;
        const auto nextIndex = (i == listElement->children.size() - 1) ? 0 : i + 1;

        auto& button = *listElement->children[i];
        button.cursorUpElement = listElement->children[prevIndex].get();
        button.cursorDownElement = listElement->children[nextIndex].get();
    }

    return listElement;
}

void GameUI::doFadeInFromBlack(float duration)
{
    actionListManager.addActionList(
        ActionList("fade in from black", fadeInFromBlackAction(duration)));
}

void GameUI::doFadeOutToBlack(float duration)
{
    actionListManager.addActionList(
        ActionList("fade in from black", fadeOutToBlackAction(duration)));
}

std::unique_ptr<Action> GameUI::fadeInFromBlackAction(float duration)
{
    using namespace actions;
    return tween(
        "Fade in from black",
        {
            .startValue = 1.f,
            .endValue = 0.f,
            .duration = duration,
            .tween = glm::exponentialEaseInOut<float>,
            .setter = [this](float v) { fadeLevel = v; },
        });
}

std::unique_ptr<Action> GameUI::fadeOutToBlackAction(float duration)
{
    using namespace actions;
    return tween(
        "Fade out to black",
        {
            .startValue = 0.f,
            .endValue = 1.f,
            .duration = duration,
            .tween = glm::exponentialEaseInOut<float>,
            .setter = [this](float v) { fadeLevel = v; },
        });
}
