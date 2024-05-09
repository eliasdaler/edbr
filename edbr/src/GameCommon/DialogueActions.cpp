#include <edbr/GameCommon/DialogueActions.h>

#include <edbr/ActionList/ActionWrappers.h>
#include <edbr/GameUI/DialogueBox.h>
#include <edbr/GameUI/IGameUI.h>
#include <edbr/Text/TextManager.h>
#include <edbr/Text/TextToken.h>

#include <fmt/format.h>

namespace
{
void prepareDialogueBox(
    TextManager& textManager,
    DialogueBox& db,
    const dialogue::TextToken& textToken,
    const LocalizedStringTag& speakerName)
{
    db.resetState();

    // speaker name
    if (!speakerName.empty()) {
        db.setSpeakerName(textManager.getString(speakerName));
    }

    // text
    db.setText(textManager.getString(textToken.text));

    // choices
    if (!textToken.choices.empty()) {
        assert(textToken.choices.size() <= DialogueBox::MaxChoices);
        for (std::size_t i = 0; i < textToken.choices.size(); ++i) {
            db.setChoiceText(i, textManager.getString(textToken.choices[i]));
        }
    }

    // voice
    if (!textToken.voiceSound.empty()) {
        db.setTempVoiceSound(textToken.voiceSound, textToken.voiceSoundSpeed);
    }
}

}

namespace actions
{
ActionList say(
    TextManager& textManager,
    IGameUI& ui,
    std::span<const dialogue::TextToken> textTokens,
    const LocalizedStringTag& speakerName)
{
    auto l = ActionList("Say");

    std::size_t textTokenIndex = 0;
    for (const auto& textToken : textTokens) {
        bool isFirst = (textTokenIndex == 0);
        bool isLast = (textTokenIndex + 1 == textTokens.size());

        // prepare
        l.addAction(doNamed("Prepare dialogue box", [&textManager, &ui, textToken, speakerName]() {
            prepareDialogueBox(
                textManager,
                ui.getDialogueBox(),
                textToken,
                textToken.name.empty() ? speakerName : textToken.name);
        }));

        // open (first only)
        if (isFirst) {
            l.addAction(doNamed("Open dialogue box", [&ui]() {
                if (!ui.isDialogueBoxOpen()) {
                    ui.openDialogueBox();
                }
            }));
        }

        // wait for dialogue input
        l.addAction(waitWhile(
            "Wait dialogue input",
            [&ui](float dt) { //
                return !ui.getDialogueBox().wantsClose();
            }));

        // close if last
        if (isLast) {
            l.addAction(doNamed("Close dialogue box", [&ui]() {
                if (ui.isDialogueBoxOpen()) {
                    ui.closeDialogueBox();
                }
            }));
        }

        // handle choice
        auto onChoice = textToken.onChoice;
        if (onChoice) {
            l.addAction(make("Dialogue choice", [&ui, onChoice]() {
                auto lastChoice = ui.getDialogueBox().getLastChoiceSelectionIndex();
                return actions::doList(onChoice(lastChoice));
            }));
        }

        ++textTokenIndex;
    }

    return l;
}

}
