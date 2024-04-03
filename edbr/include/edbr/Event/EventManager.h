#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <edbr/Event/Event.h>
#include <edbr/Event/ListenerInfo.h>

class EventManager {
public:
    using CppListenerId = intptr_t; // can convert pointer to some C++ class instance to intptr_t
    using EventQueue = std::vector<std::unique_ptr<Event>>;

public:
    struct RequestedAction {
        EventTypeId eventType;
        ListenerInfo info;

        enum Action { AddListener, RemoveListener, None };

        Action action;
    };

public:
    EventManager();

    void sendEvent(const Event& event);

    bool hasListeners(EventTypeId type) const;

    void update();
    void processRequestedActions();

    void queueEvent(std::unique_ptr<Event> e);

    void triggerEvent(const Event& event, CppListenerId senderId = 0);

    EventTypeId getEventTypeId(const char* name) const;
    const std::string& getEventTypeName(EventTypeId id) const;

    template<typename T, typename EventT>
    void addListener(T* const listener, void (T::*f)(const EventT&), bool active = true);

    template<typename EventT, typename T>
    void removeListener(T* const listener);

private:
    ListenerInfo* findListener(CppListenerId id, EventTypeId type);

    void addListener(EventTypeId type, ListenerInfo& info);
    void removeListener(EventTypeId type, ListenerInfo& info);

    // functions for C++ objects.
    void addListener(EventTypeId type, CppListenerId id, CallbackFunction& f, bool active);
    void removeListener(EventTypeId type, CppListenerId id);

    void addSubjectToListener(CppListenerId listenerId, EventTypeId type, CppListenerId subjectId);
    void removeSubjectFromListener(
        CppListenerId listenerId,
        EventTypeId type,
        CppListenerId subjectId);

    void setListenerActive(CppListenerId id, bool b);
    void setListenerActive(EventTypeId type, CppListenerId id, bool b);

    template<typename T>
    CppListenerId getCppListenerId(T* const listener);

    const std::string& getTypeName(EventTypeId type) const;

    std::unordered_map<EventTypeId, std::vector<ListenerInfo>> listeners;

    EventQueue& getActiveQueue();

    static const unsigned int NUMBER_OF_QUEUES = 2;
    EventQueue eventQueues[NUMBER_OF_QUEUES];
    unsigned int activeQueue; // index of queue which is currently used to process events
    // all other events will be added to other queue(s)

    bool processingQueue; // can't remove or add listeners here, so we add requests
    std::vector<RequestedAction> requestedActions;
};

template<typename T, typename EventT>
void EventManager::addListener(T* const listener, void (T::*f)(const EventT&), bool active)
{
    CallbackFunction func = [listener, f](const Event& event) {
        (listener->*f)(static_cast<const EventT&>(event));
    };
    addListener(EventT::GetTypeId(), getCppListenerId(listener), func, active);
}

template<typename EventT, typename T>
void EventManager::removeListener(T* const listener)
{
    removeListener(EventT::GetTypeId(), getCppListenerId(listener));
}

template<typename T>
EventManager::CppListenerId EventManager::getCppListenerId(T* const listener)
{
    return reinterpret_cast<CppListenerId>(listener);
}
