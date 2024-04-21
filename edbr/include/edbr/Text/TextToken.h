#pragma once

#include <functional>
#include <vector>

#include <edbr/Text/LocalizedStringTag.h>

class ActionList;

namespace dialogue
{
struct TextToken {
    LocalizedStringTag text;
    LocalizedStringTag name;
    std::vector<LocalizedStringTag> choices;
    std::string voiceSound;

    using ChoiceFuncType = std::function<ActionList(std::size_t choiceIndex)>;
    ChoiceFuncType onChoice;

    std::vector<std::pair<LocalizedStringTag, LocalizedStringTag>> simpleChoices;
};
}
