#include <edbr/ActionList/ActionWrappers.h>

#include <edbr/ActionList/Actions/DelayAction.h>
#include <edbr/ActionList/Actions/DoAction.h>

#include <glm/gtx/compatibility.hpp> // lerp for vec3

namespace actions
{
std::unique_ptr<Action> doNothing()
{
    return std::make_unique<DoAction>([]() {});
}

ActionList doNothingList()
{
    return ActionList("do_nothing");
}

std::unique_ptr<Action> make(MakeAction::MakeFuncType f)
{
    return std::make_unique<MakeAction>(std::move(f));
}

std::unique_ptr<Action> make(std::string name, MakeAction::MakeFuncType f)
{
    return std::make_unique<MakeAction>(std::move(name), std::move(f));
}

std::unique_ptr<Action> doNamed(std::string name, std::function<void()> f)
{
    return std::make_unique<DoAction>(std::move(name), std::move(f));
}

std::unique_ptr<Action> doList(ActionList l)
{
    return std::make_unique<ActionListFinishAction>(std::move(l));
}

std::unique_ptr<Action> delay(float delayDuration)
{
    return std::make_unique<DelayAction>(delayDuration);
}

std::unique_ptr<Action> delayForOneFrame()
{
    // TODO: make it so that it will delay for one game loop iteration,
    // not just one update call (since we can have several of them in one frame)
    return std::make_unique<WaitWhileAction>([numFramesWaited = 0](float dt) mutable {
        if (numFramesWaited < 1) {
            numFramesWaited = numFramesWaited + 1;
            return true;
        }
        return false;
    });
}

std::unique_ptr<Action> tween(TweenAction::Params params)
{
    return std::make_unique<TweenAction>(std::move(params));
}

std::unique_ptr<Action> tween(std::string name, TweenAction::Params params)
{
    return std::make_unique<TweenAction>(std::move(name), std::move(params));
}

std::unique_ptr<Action> waitWhile(WaitWhileAction::ConditionFuncType f)
{
    return std::make_unique<WaitWhileAction>(std::move(f));
}

std::unique_ptr<Action> waitWhile(std::string name, WaitWhileAction::ConditionFuncType f)
{
    return std::make_unique<WaitWhileAction>(std::move(name), std::move(f));
}

} // end of namespace actions
