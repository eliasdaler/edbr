#include <edbr/GameCommon/DialogueActions.h>

#include <edbr/ActionList/ActionWrappers.h>
#include <edbr/GameUI/DialogueBox.h>
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
    DialogueBox& dialogueBox,
    std::function<void()> openDialogueBox,
    std::function<void()> closeDialogueBox,
    const dialogue::TextToken& textToken,
    const LocalizedStringTag& speakerName)
{
    const auto dialogueDebugName = fmt::format("Dialogue: {}", textToken.text.tag);

    auto l = ActionList(
        std::move(dialogueDebugName),
        doNamed(
            "Prepare dialogue box",
            [&textManager, &dialogueBox, textToken, speakerName]() {
                prepareDialogueBox(textManager, dialogueBox, textToken, speakerName);
            }),
        doNamed("Open dialogue box", [openDialogueBox]() { openDialogueBox(); }),
        waitWhile(
            "Wait dialogue input",
            [&dialogueBox](float dt) { //
                return !dialogueBox.wantsClose();
            }),
        doNamed("Close dialogue box", [closeDialogueBox]() { closeDialogueBox(); }) //
    );

    auto onChoice = textToken.onChoice;
    if (onChoice) {
        l.addAction(make("Dialogue choice", [&dialogueBox, onChoice]() {
            auto lastChoice = dialogueBox.getLastChoiceSelectionIndex();
            return actions::doList(onChoice(lastChoice));
        }));
    }

    return l;
}

}
