#pragma once

#include <functional>
#include <string>

#include <edbr/ActionList/Action.h>

// included for convenience
#include <glm/gtx/easing.hpp>

class TweenAction : public Action {
public:
    friend class ActionListInspector;

    // tween function takes a normalized time (from 0 to 1) and returns a
    // normalized value
    using TweenFuncType = std::function<float(float time)>;
    using SetterFuncType = std::function<void(float value)>;

    struct Params {
        float startValue;
        float endValue;
        float duration;
        TweenFuncType tween;
        SetterFuncType setter;
    };

    TweenAction(Params params);
    TweenAction(std::string name, Params params);

    bool enter() override;
    bool update(float dt) override;

private:
    std::string name;
    const float startValue{};
    const float endValue{};
    const float duration{};
    const TweenFuncType tween;
    const SetterFuncType setter;

    float currentTime{};
    float currentValue{};
};
