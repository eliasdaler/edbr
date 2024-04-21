#include <edbr/ActionList/Actions/ParallelActionListsFinishAction.h>

#include <edbr/ActionList/ActionList.h>

#include <memory>

ParallelActionListsFinishAction::ParallelActionListsFinishAction() = default;

ParallelActionListsFinishAction::ParallelActionListsFinishAction(
    std::vector<ActionList> actionLists) :
    actionLists(std::move(actionLists))
{}

void ParallelActionListsFinishAction::add(ActionList list)
{
    actionLists.push_back(std::move(list));
}

void ParallelActionListsFinishAction::add(std::unique_ptr<Action> action)
{
    actionLists.push_back(ActionList("action", action));
}

bool ParallelActionListsFinishAction::enter()
{
    for (auto& al : actionLists) {
        al.play();
    }
    return isFinished();
}

bool ParallelActionListsFinishAction::update(float dt)
{
    for (auto& al : actionLists) {
        al.update(dt);
    }
    return isFinished();
}

bool ParallelActionListsFinishAction::isFinished() const
{
    for (const auto& al : actionLists) {
        if (!al.isFinished()) {
            return false;
        }
    }
    return true;
}
