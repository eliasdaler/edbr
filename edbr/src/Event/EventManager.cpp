#include <edbr/Event/EventManager.h>

#include <algorithm>
#include <cassert>

namespace util
{
// returns bool which indicates if item was removed
template<typename C, typename F>
bool remove_if(C& c, F f)
{
    const auto it = std::remove_if(std::begin(c), std::end(c), f);
    bool found = (it != c.end());
    c.erase(it, std::end(c));
    return found;
}

}

EventManager::EventManager() : activeQueue(0), processingQueue(false)
{}

void EventManager::sendEvent(const Event& event)
{
    auto listenerListIt = listeners.find(event.getTypeId());
    if (listenerListIt != listeners.end()) {
        auto& eventListeners = listenerListIt->second; // list of listeners who listen to this
                                                       // event
        for (auto& listener : eventListeners) {
            if (listener.isActive()) {
                if (listener.listensTo(event.getSenderId())) {
                    listener.callFunction(event);
                }
            }
        }
    }
}

void EventManager::update()
{
    auto& queue = getActiveQueue();
    if (queue.size() != 0) {
        activeQueue = (activeQueue + 1) % NUMBER_OF_QUEUES; // swap active queues
        processingQueue = true;
        for (const auto& ePtr : queue) {
            sendEvent(*ePtr);
        }
        processingQueue = false;

        queue.clear();
    }

    processRequestedActions();
}

void EventManager::processRequestedActions()
{
    for (auto& a : requestedActions) {
        switch (a.action) {
        case RequestedAction::Action::AddListener:
            // TODO
            break;
        case RequestedAction::Action::RemoveListener:
            removeListener(a.eventType, a.info);
            break;
        default:
            break;
        }
    }

    requestedActions.clear();
}

void EventManager::queueEvent(std::unique_ptr<Event> e)
{
    getActiveQueue().push_back(std::move(e));
}

void EventManager::triggerEvent(const Event& event, CppListenerId /*senderId*/)
{
    sendEvent(event);
}

bool EventManager::hasListeners(EventTypeId type) const
{
    auto listenersListIt = listeners.find(type);
    return listenersListIt != listeners.end();
}

void EventManager::addListener(EventTypeId type, CppListenerId id, CallbackFunction& f, bool active)
{
    ListenerInfo info;
    info.id = id;
    info.callback = f;
    info.active = active;
    addListener(type, info);
}

void EventManager::addListener(EventTypeId type, ListenerInfo& info)
{
    if (processingQueue) {
        requestedActions.push_back(
            RequestedAction{type, info, RequestedAction::Action::AddListener});
        return;
    }

    auto& listenerList = listeners[type];

    // check if tried to add same observer twice
    bool addedAlready = false;

    if (findListener(info.id, type)) { // adding other thing
        addedAlready = true;
    }

    assert(!addedAlready);

    listenerList.push_back(std::move(info));
}

void EventManager::removeListener(EventTypeId type, ListenerInfo& info)
{
    if (processingQueue) {
        requestedActions.push_back(
            RequestedAction{type, info, RequestedAction::Action::RemoveListener});
        return;
    }

    if (auto findIt = listeners.find(type); findIt != listeners.end()) {
        auto& listenerList = findIt->second;
        bool removed =
            util::remove_if(listenerList, [&info](const auto& elem) { return info.id == elem.id; });
        assert(removed && "Listener didn't subscribe to event of this type before");
        if (listenerList.empty()) {
            listeners.erase(findIt);
        }
    }
}

void EventManager::addSubjectToListener(
    CppListenerId listenerId,
    EventTypeId type,
    CppListenerId subjectId)
{
    auto listenerPtr = findListener(listenerId, type);
    assert(listenerPtr); // listener not found!

    auto& listener = *listenerPtr;
    listener.addSubject(subjectId);
}

void EventManager::removeSubjectFromListener(
    CppListenerId listenerId,
    EventTypeId type,
    CppListenerId subjectId)
{
    auto listenerPtr = findListener(listenerId, type);
    assert(listenerPtr); // listener not found!

    auto& listener = *listenerPtr;
    listener.removeSubject(subjectId);
}

void EventManager::removeListener(EventTypeId type, CppListenerId id)
{
    ListenerInfo info;
    info.id = id;
    removeListener(type, info);
}

// used to temporarly disable listeners
void EventManager::setListenerActive(CppListenerId id, bool b)
{
    for (auto& pair : listeners) {
        auto& listenerList = pair.second;
        for (auto& listener : listenerList) {
            if (listener.id == id) {
                listener.setActive(b);
            }
        }
    }
}

void EventManager::setListenerActive(EventTypeId type, CppListenerId id, bool b)
{
    auto listenerPtr = findListener(id, type);
    assert(listenerPtr); // listener not found!

    auto& listener = *listenerPtr;
    listener.setActive(b);
}

EventManager::EventQueue& EventManager::getActiveQueue()
{
    return eventQueues[activeQueue];
}

ListenerInfo* EventManager::findListener(CppListenerId id, EventTypeId type)
{
    auto listenerListIt = listeners.find(type);
    if (listenerListIt != listeners.end()) {
        auto& listenerList = listenerListIt->second;
        const auto listenerIt =
            std::find_if(listenerList.begin(), listenerList.end(), [&id](const ListenerInfo& info) {
                return info.id == id;
            });
        if (listenerIt != listenerList.end()) {
            return &(*listenerIt);
        }
    }
    return nullptr;
}
