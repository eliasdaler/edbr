#pragma once

#include <cstdint> // intptr_t
#include <functional>
#include <set>

class Event;
using CallbackFunction = std::function<void(const Event&)>;

struct ListenerInfo {
    using CppListenerId = std::intptr_t;
    using EntityListenerId = int;
    ListenerInfo();

    void addSubject(CppListenerId subjectId);
    void removeSubject(CppListenerId subjectId);

    bool listensTo(CppListenerId senderId) const;

    void callFunction(const Event& event);

    bool isActive() const;
    void setActive(bool b);

    // data
    intptr_t id;
    bool active;
    std::set<CppListenerId> subjects; // if empty, listener will receive events from everyone
    CallbackFunction callback;

    bool temporary; // removed after one callback call;
    bool callbackCalled;
};
