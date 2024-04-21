#pragma once

#include <string>

class Action;
class ActionList;

class ActionListInspector {
public:
    void showActionListInfo(const ActionList& actionList, bool rootList = false);

private:
    void showAction(const Action& action, bool current);
    std::string getActionTypeName(const Action& action);
};
