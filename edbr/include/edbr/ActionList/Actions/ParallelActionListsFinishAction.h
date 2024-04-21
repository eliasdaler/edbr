#pragma once

#include <memory>
#include <vector>

#include <edbr/ActionList/Action.h>
#include <edbr/ActionList/ActionList.h>

// ParallelActionListsFinishAction is used when you want to play several lists
// in parallel and wait for them all to finish
class ParallelActionListsFinishAction : public Action {
public:
    ParallelActionListsFinishAction();
    ParallelActionListsFinishAction(std::vector<ActionList> actionList);

    void add(ActionList list);
    void add(std::unique_ptr<Action> action);

    bool enter() override;
    bool update(float dt) override;

private:
    bool isFinished() const;

    std::vector<ActionList> actionLists;
};
