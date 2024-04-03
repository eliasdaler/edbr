#pragma once

#include <string>

#include <edbr/Event/Event.h>

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

struct EntityAnimationEvent : public EventBase<EntityAnimationEvent> {
    entt::handle entity;
    std::string event;
};
