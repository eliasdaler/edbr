#include <edbr/ActionList/Actions/WaitWhileAction.h>

#include <edbr/ActionList/ActionList.h>

WaitWhileAction::WaitWhileAction(ConditionFuncType f) : f(std::move(f))
{}

WaitWhileAction::WaitWhileAction(std::string conditionName, ConditionFuncType f) :
    f(std::move(f)), conditionName(std::move(conditionName))
{}

bool WaitWhileAction::enter()
{
    bool res = f(0);
    return !res;
}

bool WaitWhileAction::update(float dt)
{
    bool res = f(dt);
    return !res;
}
