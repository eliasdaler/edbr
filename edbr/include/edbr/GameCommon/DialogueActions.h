#pragma once

#include <functional>

#include <edbr/Text/LocalizedStringTag.h>

class ActionList;
class DialogueBox;
class TextManager;

namespace dialogue
{
struct TextToken;
}

namespace actions
{
ActionList say(
    TextManager& textManager,
    DialogueBox& dialogueBox,
    std::function<void()> openDialogueBox,
    std::function<void()> closeDialogueBox,
    const dialogue::TextToken& textToken,
    const LocalizedStringTag& speakerName);
}
