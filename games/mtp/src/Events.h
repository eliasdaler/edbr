#pragma once

#include <string>

#include <edbr/Event/Event.h>

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

struct EntityAnimationEvent : public EventBase<EntityAnimationEvent> {
    entt::handle entity;
    std::string event;
};

struct CharacterCollisionStartedEvent : public EventBase<CharacterCollisionStartedEvent> {
    entt::handle entity;
};

struct CharacterCollisionEndedEvent : public EventBase<CharacterCollisionEndedEvent> {
    entt::handle entity;
};

struct EntityTeleportedEvent : public EventBase<EntityTeleportedEvent> {
    entt::handle entity;
};
