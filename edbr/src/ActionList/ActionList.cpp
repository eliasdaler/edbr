#include <edbr/ActionList/ActionList.h>

#include <cassert>

#include <edbr/ActionList/Actions/ActionListFinishAction.h>
#include <edbr/ActionList/Actions/DoAction.h>

ActionList::ActionList() : ActionList("empty")
{}

ActionList::ActionList(std::string name) : name(std::move(name))
{}

ActionList::ActionList(std::string name, std::vector<std::unique_ptr<Action>> actions) :
    name(std::move(name)), actions(std::move(actions))
{}

void ActionList::addAction(std::unique_ptr<Action> action)
{
    actions.push_back(std::move(action));
}

void ActionList::addAction(std::function<void()> action)
{
    actions.push_back(std::make_unique<DoAction>(std::move(action)));
}

void ActionList::addAction(ActionList actionList)
{
    actions.push_back(std::make_unique<ActionListFinishAction>(std::move(actionList)));
}

bool ActionList::isFinished() const
{
    if (looping) {
        return false;
    }

    return currentIdx >= actions.size();
}

void ActionList::update(float dt)
{
    assert(wasPlayCalled && "play() not called before update!");
    if (isFinished()) {
        return;
    }

    auto& action = *actions[currentIdx];
    if (bool finished = action.update(dt); finished) {
        goToNextAction();
    }
}

void ActionList::play()
{
    wasPlayCalled = true;
    currentIdx = 0;
    stopped = true;
    goToNextAction();
}

void ActionList::advanceIndex()
{
    ++currentIdx;
    if (currentIdx >= actions.size() && looping) {
        currentIdx = 0;
    }
}

void ActionList::goToNextAction()
{
    while (!isFinished()) {
        if (stopped) {
            assert(currentIdx == 0);
            stopped = false;
        } else {
            advanceIndex();
        }

        if (isFinished()) {
            return;
        }

        auto& action = *actions[currentIdx];
        const auto finished = action.enter();
        if (!finished) {
            return;
        }
    }
}
