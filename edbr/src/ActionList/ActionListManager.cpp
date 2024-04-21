#include <edbr/ActionList/ActionListManager.h>

#include <cassert>

#include <fmt/format.h>
#include <stdexcept>

void ActionListManager::update(float dt, bool gamePaused)
{
    for (auto& [name, al] : actionLists) {
        if (gamePaused && !al.shouldRunWhenGameIsPaused()) {
            continue;
        }
        al.update(dt);
    }
    // remove finished lists
    std::erase_if(actionLists, [](const auto& p) { return p.second.isFinished(); });
}

void ActionListManager::addActionList(ActionList actionList)
{
    actionList.play();
    if (!actionList.isFinished()) {
        if (isActionListPlaying(actionList.getName())) {
            throw std::runtime_error(fmt::format(
                "action list with name '{}' is already playing, call stopActionList first",
                actionList.getName()));
        }
        actionLists.emplace(actionList.getName(), std::move(actionList));
    }
}

void ActionListManager::stopActionList(const std::string& actionListName)
{
    auto it = actionLists.find(actionListName);
    assert(it != actionLists.end());
    actionLists.erase(it);
};

bool ActionListManager::isActionListPlaying(const std::string& actionListName) const
{
    return actionLists.contains(actionListName);
};
