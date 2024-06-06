#pragma once

#include <span>

#include <edbr/Text/LocalizedStringTag.h>

class ActionList;
class TextManager;
class IGameUI;

namespace dialogue
{
struct TextToken;
}

namespace actions
{
ActionList say(
    TextManager& textManager,
    IGameUI& ui,
    std::span<const dialogue::TextToken> textTokens,
    const LocalizedStringTag& speakerName);
}
