#pragma once

#include "LevelScript.h"

#include "Game.h"

#include "MTPSaveFile.h"

class CityLevelScript : public LevelScript {
public:
    CityLevelScript(Game& game) : LevelScript(game) {}

    InteractResult onEntityInteract(entt::handle e, const std::string& name) override
    {
        if (name == "NpcNearStore") {
            auto& save = game.getSaveFile();
            if (save.getData<bool>("talked_to_dude")) {
                return LST{"DUDE_DIALOGUE"};
            } else {
                save.setData("talked_to_dude", true);
                return LST{"DUDE_DIALOGUE_INTRO"};
            }
        }

        if (name == "NpcOnOverpass") {
            return LST{"ELLIPSIS"};
        }

        if (name == "Store") {
            return LST{"STORE_CLOSED_NOTE_TEXT"};
        }

        if (name == "FrontDoor") {
            game.stopPlayerMovement();
            game.changeLevel("house", "House", Game::LevelTransitionType::EnterDoor);
            return {};
        }

        return {};
    }
};
