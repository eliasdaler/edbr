#pragma once

#include <edbr/ActionList/Action.h>

#include <edbr/ActionList/ActionList.h>

class ActionListFinishAction : public Action {
public:
    ActionListFinishAction(ActionList actionList);

    bool enter() override;
    bool update(float dt) override;

    const ActionList& getActionList() const { return actionList; }

private:
    ActionList actionList;
};
