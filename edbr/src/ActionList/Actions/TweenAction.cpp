#include <edbr/ActionList/Actions/TweenAction.h>

#include <cassert>
#include <cmath>

TweenAction::TweenAction(Params params) : TweenAction("", params)
{}

TweenAction::TweenAction(std::string name, Params params) :
    name(std::move(name)),
    startValue(params.startValue),
    endValue(params.endValue),
    duration(params.duration),
    tween(std::move(params.tween)),
    setter(std::move(params.setter))
{
    assert(tween && "tween func can't be null");
    assert(duration >= 0.f && "duration should be >=0");
    assert(setter && "setter func can't be null");
}

bool TweenAction::enter()
{
    currentTime = 0.f;
    currentValue = startValue;
    setter(currentValue);

    return startValue == endValue || duration == 0.f;
}

bool TweenAction::update(float dt)
{
    currentTime += dt;
    if (currentTime >= duration) {
        currentValue = endValue;
        setter(currentValue);
        currentTime = duration;
        return true;
    }

    auto t = tween(currentTime / duration);
    currentValue = std::lerp(startValue, endValue, t);
    setter(currentValue);
    return false;
}
