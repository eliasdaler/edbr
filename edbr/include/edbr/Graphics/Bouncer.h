#pragma once

#include <functional>

#include <glm/gtx/easing.hpp>

class Bouncer {
public:
    using TweenFuncType = std::function<float(float time)>;

    struct Params {
        float maxOffset{0.f};
        float moveDuration{0.f};
        TweenFuncType tween = glm::linearInterpolation<float>;
    };

public:
    Bouncer() = default;
    Bouncer(Params params);
    void update(float dt);
    float getOffset() const { return offset; };

private:
    float maxOffset = 0.f;
    float moveDuration = 0.f; // how quickly it moves from -offset to offset
    TweenFuncType tween;

    float offset = 0.f; // current offset from the starting position
    float currentTime = 0.f;
    bool moveToMax = true; // if true, moves from -offset to offset, otherwise it moves backwards
};
