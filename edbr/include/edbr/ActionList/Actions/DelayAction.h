#pragma once

#include <edbr/ActionList/Action.h>

class DelayAction : public Action {
public:
    DelayAction(float delayDuration);

    bool enter() override;
    bool update(float dt) override;

    bool isFinished() const;

    float getCurrentTime() const { return currentTime; }
    float getDelay() const { return delayDuration; }

private:
    float currentTime{0.f};
    float delayDuration{0.f};
};
