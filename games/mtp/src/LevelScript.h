#pragma once

#include <string>
#include <variant>

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include <edbr/ActionList/ActionList.h>
#include <edbr/Text/LocalizedStringTag.h>
#include <edbr/Text/TextToken.h>

class Game;

class LevelScript {
public:
    LevelScript(Game& game) : game(game) {}
    virtual ~LevelScript() = default;

    virtual void onLevelEnter() {}
    virtual void update(float dt){};
    virtual void onLevelExit() {}

    using InteractResult = std::variant<
        std::monostate,
        LocalizedStringTag, // dialogue text
        std::vector<LocalizedStringTag>, // multiple texts
        std::vector<dialogue::TextToken>, // more dialogue text
        ActionList // action list
        >;

    virtual InteractResult onEntityInteract(entt::handle e, const std::string& name) { return {}; }
    virtual void onTriggerEnter(entt::handle trigger, const std::string& name) {}
    virtual void onTriggerExit(entt::handle trigger, const std::string& name) {}

protected:
    Game& game;
};
