#pragma once

#include "LevelScript.h"

#include "Game.h"

#include "EntityUtil.h"

class HouseLevelScript : public LevelScript {
public:
    HouseLevelScript(Game& game) : LevelScript(game) {}

    void onLevelEnter() override
    {
        auto diaryInteract = entityutil::
            findEntityBySceneNodeName(game.getEntityRegistry(), "Interact.Sphere.Diary");
        diaryInteract.get<InteractComponent>().type = InteractComponent::Type::Save;
    }

    InteractResult onEntityInteract(entt::handle e, const std::string& name) override
    {
        if (name == "TV") {
            game.playSound("assets/sounds/spooky.wav");
            auto token = dialogue::TextToken{
                .text = LST{"GLEEBY_DEEBY_TV"},
                .choices = {LST{"TURN_TV_OFF"}, LST{"WELCOME_OVERLORDS"}},
            };
            token.onChoice = [this](std::size_t index) {
                auto tt = dialogue::TextToken{
                    // TODO: move to NPC component?
                    .name = LST{"GLEEBY_DEEBY_NAME"},
                    .voiceSound = "assets/sounds/ui/meow.wav",
                    .voiceSoundSpeed = 0.15f,
                };

                if (index == 0) {
                    tt.text = LST{"WELCOME_REACTION"};
                    return game.say(tt);
                }

                std::vector<dialogue::TextToken> tokens{2, tt};
                tokens[0].text = LST{"TURN_TV_OFF_REACTION_1"};
                tokens[1].text = LST{"TURN_TV_OFF_REACTION_2"};
                return game.say(tokens);
            };
            return std::vector{token};
        }

        if (name == "Bed") {
            return LST{"BED_NOT_TIRED"};
        }

        if (name == "Diary") {
            return game.saveGameSequence();
        }

        return {};
    }

    void onTriggerEnter(entt::handle e, const std::string& name) override
    {
        if (name == "Forest") {
            game.stopPlayerMovement();
            game.changeLevel("city", "ExitAppartment", Game::LevelTransitionType::EnterDoor);
        }

        if (name == "CameraTV") {
            game.setCurrentCamera("TV", 1.5f);
        }

        if (name == "CameraWindow") {
            game.setCurrentCamera("Window", 1.5f);
        }
    }

    void onTriggerExit(entt::handle e, const std::string& name) override
    {
        if (name == "CameraTV" || name == "CameraWindow") {
            game.setCurrentCamera("Default", 1.5f);
        }
    }
};
