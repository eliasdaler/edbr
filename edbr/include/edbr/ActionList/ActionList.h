#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <edbr/ActionList/Action.h>

// ActionList is a list of actions which are performed one after another.
// This pattern allows to easily to multi-frame actions in sequence without
// writing state machines or "hasActionCompleted" booleans.
class ActionList {
public:
    ActionList();
    ActionList(std::string name);
    ActionList(std::string name, std::vector<std::unique_ptr<Action>> actions);

    // Move only (because we want to move actions, not copy them)
    ActionList(ActionList&&) = default;
    ActionList& operator=(ActionList&&) = default;
    ActionList(const ActionList&) = delete;
    ActionList& operator=(const ActionList&) = delete;

    template<typename... Args>
    ActionList(std::string name, Args&&... args);

    template<typename... Args>
    void addActions(Args&&... args);

    void addAction(std::unique_ptr<Action> action);
    void addAction(std::function<void()> action);
    void addAction(ActionList actionList);

    void play(); // starts playback of an action list from the beginning

    void setName(std::string n) { name = std::move(n); }
    const std::string& getName() const { return name; }

    void update(float dt);
    bool isFinished() const;

    void setLooping(bool b) { looping = b; }
    bool isLooping() const { return looping; }

    bool isEmpty() { return actions.empty(); }

    void setRunWhenGameIsPaused(bool b) { runWhenGameIsPaused = b; }
    bool shouldRunWhenGameIsPaused() const { return runWhenGameIsPaused; }

    const std::vector<std::unique_ptr<Action>>& getActions() const { return actions; }
    std::size_t getCurrentActionIndex() const { return currentIdx; }

    bool isPlaying() const { return !isFinished() && wasPlayCalled; }

private:
    bool runWhenGameIsPaused{false};

    void goToNextAction();
    void advanceIndex();

    std::string name;
    std::vector<std::unique_ptr<Action>> actions;
    std::size_t currentIdx{0};
    bool stopped{true}; // if true, goToNextAction() will start with the first action
    bool wasPlayCalled{false};
    bool looping{false};
};

template<typename... Args>
ActionList::ActionList(std::string name, Args&&... args) : name(std::move(name))
{
    actions.reserve(sizeof...(Args));
    addActions(std::forward<Args>(args)...);
}

template<typename... Args>
void ActionList::addActions(Args&&... args)
{
    (addAction(std::move(args)), ...);
}
