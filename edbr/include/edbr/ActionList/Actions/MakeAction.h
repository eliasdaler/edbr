#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <string>

#include <edbr/ActionList/Action.h>

// MakeAction is used when you need to branch dynamically during the execution of
// action list, for example:
//
//    return ActionList(
//        "some action list",
//        make([]() { //
//            if (someCondition) {
//                return doAnotherAction();
//            }
//            return doOneAction();
//        }));
//
class MakeAction : public Action {
public:
    using MakeFuncType = std::function<std::unique_ptr<Action>()>;
    MakeAction(MakeFuncType makeFunc);
    MakeAction(std::string makeFuncName, MakeFuncType makeFunc);

    bool enter() override;
    bool update(float dt) override;

    bool actionMade() const { return action != nullptr; }
    const Action& getAction() const
    {
        assert(actionMade());
        return *action;
    }

    const std::string& getMakeFuncName() const { return makeFuncName; }

private:
    MakeFuncType makeFunc;
    std::string makeFuncName;
    std::unique_ptr<Action> action;
};
