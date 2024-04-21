#pragma once

#include <memory>

#include <edbr/ActionList/Action.h>
#include <edbr/ActionList/ActionList.h>
#include <edbr/ActionList/Actions/ActionListFinishAction.h>

#include <edbr/ActionList/Actions/MakeAction.h>
#include <edbr/ActionList/Actions/ParallelActionListsFinishAction.h>
#include <edbr/ActionList/Actions/TweenAction.h>
#include <edbr/ActionList/Actions/WaitWhileAction.h>

// Useful wrappers to simplify creation of actions
// Usage:
//
//     using namespace actions
//     ActionList l("someList", delay(0.5f), tween(...));
namespace actions
{
std::unique_ptr<Action> doNothing();
ActionList doNothingList();
std::unique_ptr<Action> make(MakeAction::MakeFuncType f);
std::unique_ptr<Action> make(std::string name, MakeAction::MakeFuncType f);
std::unique_ptr<Action> doNamed(std::string name, std::function<void()> f);
std::unique_ptr<Action> doList(ActionList l);
std::unique_ptr<Action> delay(float delayDuration);
std::unique_ptr<Action> delayForOneFrame();
std::unique_ptr<Action> tween(TweenAction::Params params);
std::unique_ptr<Action> tween(std::string name, TweenAction::Params params);
std::unique_ptr<Action> waitWhile(WaitWhileAction::ConditionFuncType f);
std::unique_ptr<Action> waitWhile(std::string name, WaitWhileAction::ConditionFuncType f);

// do lists in parallel, proceed when all of them complete
template<typename... Args>
std::unique_ptr<Action> parallel(Args&&... args)
{
    auto pl = std::make_unique<ParallelActionListsFinishAction>();
    (pl->add(std::move(args)), ...);
    return pl;
}

// do multiple action lists one after another
template<typename... Args>
std::unique_ptr<Action> sequence(Args&&... args)
{
    auto l = ActionList("sequence", std::move(args)...);
    return std::make_unique<ActionListFinishAction>(std::move(l));
}

}
