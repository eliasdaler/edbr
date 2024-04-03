#include <edbr/Event/ListenerInfo.h>

#include <cassert>

ListenerInfo::ListenerInfo() : id(0), active(false), temporary(false), callbackCalled(false)
{}

void ListenerInfo::callFunction(const Event& event)
{
    if (temporary) {
        callbackCalled = true;
    }

    callback(event);
}

bool ListenerInfo::isActive() const
{
    return active;
}

void ListenerInfo::setActive(bool b)
{
    active = b;
}

void ListenerInfo::addSubject(CppListenerId subjectId)
{
    assert(subjects.find(subjectId) == subjects.end() && "Subject is already added");
    subjects.insert(subjectId);
}

void ListenerInfo::removeSubject(CppListenerId subjectId)
{
    assert(subjects.find(subjectId) != subjects.end() && "Subject is not in subject list");
    subjects.erase(subjectId);
}

bool ListenerInfo::listensTo(CppListenerId senderId) const
{
    return subjects.empty() || (subjects.find(senderId) != subjects.end());
}
