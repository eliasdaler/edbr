#include <Game.h>

#include <edbr/ECS/Components/NPCComponent.h>
#include <edbr/ECS/Components/SpriteAnimationComponent.h>
#include <edbr/ECS/Components/SpriteComponent.h>

#include "Components.h"

#include <edbr/GameCommon/EntityUtil2D.h>

void Game::entityPostInit(entt::handle e)
{
    if (auto scPtr = e.try_get<SpriteComponent>(); scPtr) {
        const auto imageId = gfxDevice.loadImageFromFile(scPtr->spritePath);
        const auto& image = gfxDevice.getImage(imageId);
        scPtr->sprite.setTexture(image);
        scPtr->sprite.pivot = {0.5f, 1.f};
    }

    if (auto acPtr = e.try_get<SpriteAnimationComponent>(); acPtr) {
        auto& ac = *acPtr;
        entityutil::setSpriteAnimation(e, ac.defaultAnimationName);
    }

    // For NPCs, add InteractComponent with "Talk" type if not added already
    if (e.all_of<NPCComponent>() && !e.all_of<InteractComponent>()) {
        auto& ic = e.emplace<InteractComponent>();
        ic.type = InteractComponent::Type::Talk;
    }
}
