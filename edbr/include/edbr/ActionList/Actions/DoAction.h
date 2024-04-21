#pragma once

#include <functional>
#include <string>

#include <edbr/ActionList/Action.h>

class DoAction : public Action {
public:
    DoAction(std::function<void()> f);
    DoAction(std::string name, std::function<void()> f);

    bool enter() override;

    const std::string& getName() const { return name; }

private:
    std::string name;
    std::function<void()> f;
};
