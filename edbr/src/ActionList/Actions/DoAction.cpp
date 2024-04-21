#include <edbr/ActionList/Actions/DoAction.h>

DoAction::DoAction(std::function<void()> f) : f(std::move(f))
{}

DoAction::DoAction(std::string name, std::function<void()> f) :
    name(std::move(name)), f(std::move(f))
{}

bool DoAction::enter()
{
    f();
    return true;
}
