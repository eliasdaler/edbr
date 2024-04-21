#include <edbr/ActionList/Actions/ActionListFinishAction.h>

#include <edbr/ActionList/ActionList.h>

ActionListFinishAction::ActionListFinishAction(ActionList actionList) :
    actionList(std::move(actionList))
{}

bool ActionListFinishAction::enter()
{
    actionList.play();
    return actionList.isFinished();
}

bool ActionListFinishAction::update(float dt)
{
    actionList.update(dt);
    return actionList.isFinished();
}
