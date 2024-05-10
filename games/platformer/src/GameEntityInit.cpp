#include <Game.h>

#include <edbr/ECS/Components/SpriteAnimationComponent.h>
#include <edbr/ECS/Components/SpriteComponent.h>

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
        auto& sc = e.get<SpriteComponent>();

        ac.animator.setAnimation(
            ac.animationsData->getAnimation(ac.defaultAnimationName), ac.defaultAnimationName);
        ac.animator.animate(sc.sprite, ac.animationsData->getSpriteSheet());
    }
}
