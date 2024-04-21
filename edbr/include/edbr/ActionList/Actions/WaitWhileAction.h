#pragma once

#include <functional>
#include <string>

#include <edbr/ActionList/Action.h>

// WaitWhileAction is used for waiting for some condition to become false
// example:
//
//    int someInt = 0;
//    return doList(ActionList(
//        "do stuff",
//        // will execute this 10 times
//        waitWhile([someInt](float dt) {
//            ++someInt;
//            return someInt < 10; //
//        })));
//
class WaitWhileAction : public Action {
public:
    using ConditionFuncType = std::function<bool(float dt)>;

    WaitWhileAction(ConditionFuncType f);
    WaitWhileAction(std::string conditionName, ConditionFuncType f);

    bool enter() override;
    bool update(float dt) override;

    const std::string& getConditionName() const { return conditionName; }

private:
    ConditionFuncType f;
    std::string conditionName;
};
