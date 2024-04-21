#include <edbr/ActionList/Actions/MakeAction.h>

#include <cassert>

MakeAction::MakeAction(MakeFuncType makeFunc) : makeFunc(std::move(makeFunc))
{}

MakeAction::MakeAction(std::string makeFuncName, MakeFuncType makeFunc) :
    makeFunc(std::move(makeFunc)), makeFuncName(std::move(makeFuncName))
{}

bool MakeAction::enter()
{
    action = makeFunc();
    if (!action) {
        return true;
    }
    return action->enter();
}

bool MakeAction::update(float dt)
{
    assert(action);
    return action->update(dt);
}
