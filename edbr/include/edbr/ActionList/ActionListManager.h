#pragma once

#include <string>
#include <unordered_map>

#include <edbr/ActionList/ActionList.h>

class ActionListManager {
public:
    using ActionListMap = std::unordered_map<std::string, ActionList>;

public:
    void update(float dt, bool gamePaused);

    void addActionList(ActionList actionList);

    void stopActionList(const std::string& actionListName);
    bool isActionListPlaying(const std::string& actionListName) const;

    const ActionListMap& getActionLists() const { return actionLists; }

private:
    ActionListMap actionLists;
};
