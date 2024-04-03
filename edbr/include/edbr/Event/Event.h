#pragma once

#include <limits>

using EventTypeId = int;

using EventActorId = std::size_t;
static constexpr EventActorId NULL_EVENT_ACTOR_ID = std::numeric_limits<EventActorId>::max();

class Event {
public:
    virtual ~Event();
    virtual EventTypeId getTypeId() const = 0;

    void setSenderId(EventActorId id);
    EventActorId getSenderId() const { return senderId; }

    void setReceiverId(EventActorId id);
    EventActorId getReceiverId() const { return receiverId; }

protected:
    static int lastId;

    EventActorId senderId{NULL_EVENT_ACTOR_ID};
    EventActorId receiverId{NULL_EVENT_ACTOR_ID};
};

template<typename T>
class EventBase : public Event {
public:
    static int GetTypeId() { return typeId; }
    int getTypeId() const override { return typeId; }

private:
    static const int typeId;
};

template<typename T>
const int EventBase<T>::typeId = Event::lastId++;
