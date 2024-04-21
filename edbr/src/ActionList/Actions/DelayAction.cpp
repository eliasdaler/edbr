#include <edbr/ActionList/Actions/DelayAction.h>

#include <cassert>

DelayAction::DelayAction(float delayDuration) : delayDuration(delayDuration)
{
    assert(delayDuration >= 0.f);
}

bool DelayAction::enter()
{
    currentTime = 0.f;
    return isFinished();
}

bool DelayAction::update(float dt)
{
    currentTime += dt;
    return isFinished();
}

bool DelayAction::isFinished() const
{
    return currentTime >= delayDuration;
}
