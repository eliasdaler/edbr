#include <edbr/GameUI/Cursor.h>

#include <edbr/Audio/AudioManager.h>
#include <edbr/Graphics/SpriteRenderer.h>
#include <edbr/Input/ActionMapping.h>
#include <edbr/UI/Element.h>
#include <edbr/UI/InputEvent.h>

void Cursor::handleInput(const ActionMapping& am, AudioManager& audioManager)
{
    static const auto cursorUpAction = am.getActionTagHash("CursorUp");
    static const auto cursorDownAction = am.getActionTagHash("CursorDown");
    static const auto cursorLeftAction = am.getActionTagHash("CursorLeft");
    static const auto cursorRightAction = am.getActionTagHash("CursorRight");
    static const auto menuConfirmAction = am.getActionTagHash("MenuConfirm");
    static const auto menuBackAction = am.getActionTagHash("MenuBack");

    static const auto allActions = std::array{
        cursorUpAction,
        cursorDownAction,
        cursorLeftAction,
        cursorRightAction,
        menuConfirmAction,
        menuBackAction,
    };
    static const std::unordered_map<ActionTagHash, ui::ActionType> stateMapping{
        {cursorUpAction, ui::ActionType::Up},
        {cursorDownAction, ui::ActionType::Down},
        {cursorLeftAction, ui::ActionType::Left},
        {cursorRightAction, ui::ActionType::Right},
        {menuConfirmAction, ui::ActionType::Confirm},
        {menuBackAction, ui::ActionType::Back},
    };

    bool inputHandled = false;
    const auto& inputHandler = selection->inputHandler;
    if (inputHandler) {
        for (const auto& action : allActions) {
            const auto actionType = stateMapping.at(action);
            if (am.wasJustPressed(action)) {
                inputHandled = inputHandler(
                    *selection,
                    ui::InputEvent{
                        .actionState = ui::ActionState::Pressed,
                        .actionType = actionType,
                    });
            } else if (am.wasJustReleased(action)) {
                inputHandled = inputHandler(
                    *selection,
                    ui::InputEvent{
                        .actionState = ui::ActionState::Released,
                        .actionType = actionType,
                    });
            } else if (am.isHeld(action)) {
                inputHandled = inputHandler(
                    *selection,
                    ui::InputEvent{
                        .actionState = ui::ActionState::Held,
                        .actionType = actionType,
                        .timeHeld = am.getTimePressed(action),
                    });
            }
            if (inputHandled) {
                break;
            }
        }
    }

    if (!inputHandled) {
        bool cursorMoved = false;
        auto prevSelection = selection;
        if (am.wasJustPressed(cursorUpAction)) {
            if (selection->cursorUpElement) {
                selection = selection->cursorUpElement;
                cursorMoved = true;
            }
        } else if (am.wasJustPressed(cursorDownAction)) {
            if (selection->cursorDownElement) {
                selection = selection->cursorDownElement;
                cursorMoved = true;
            }
        }
        if (am.wasJustPressed(menuConfirmAction) && !selection->enabled) {
            if (!errorSound.empty()) {
                audioManager.playSound(errorSound);
            }
        }

        if (cursorMoved) {
            prevSelection->setSelected(false);
            selection->setSelected(true);
            if (!moveSound.empty()) {
                audioManager.playSound(moveSound);
            }
        }
    }
}

void Cursor::update(float dt)
{
    if (selection) {
        bool wasCursorVisible = visible;
        visible = selection->visible;
        if (visible && !wasCursorVisible) {
            selection->setSelected(true);
        }
        bouncer.update(dt);
    }
}

void Cursor::draw(SpriteRenderer& spriteRenderer) const
{
    if (selection) {
        const auto cursorPos = selection->absolutePosition + selection->cursorSelectionOffset +
                               glm::vec2{bouncer.getOffset(), 0.f};
        spriteRenderer.drawSprite(sprite, cursorPos, 0.f, spriteScale);
    }
}
