#include <edbr/GameCommon/CommonComponentLoaders2D.h>

#include <edbr/Core/JsonDataLoader.h>
#include <edbr/ECS/ComponentFactory.h>

#include <edbr/ECS/Components/CollisionComponent2D.h>
#include <edbr/ECS/Components/SpriteAnimationComponent.h>
#include <edbr/ECS/Components/SpriteComponent.h>

namespace edbr
{
void registerCollisionComponent2DLoader(ComponentFactory& cf)
{
    cf.registerComponentLoader(
        "collision", [](entt::handle e, CollisionComponent2D& cc, const JsonDataLoader& loader) {
            loader.getIfExists("size", cc.size);
            loader.getIfExists("origin", cc.origin);
        });
}

void registerSpriteComponentLoader(ComponentFactory& cf)
{
    cf.registerComponentLoader(
        "sprite", [](entt::handle e, SpriteComponent& sc, const JsonDataLoader& loader) {
            loader.get("texture", sc.spritePath);
            loader.get("z", sc.z, 0);
        });
}

void registerSpriteAnimationComponentLoader(
    ComponentFactory& cf,
    const std::unordered_map<std::string, SpriteAnimationData>& animationsData)
{
    cf.registerComponentLoader(
        "animation",
        [&animationsData](
            entt::handle e, SpriteAnimationComponent& ac, const JsonDataLoader& loader) {
            loader.get("data", ac.animationsDataTag);

            auto it = animationsData.find(ac.animationsDataTag);
            if (it == animationsData.end()) {
                throw std::runtime_error(
                    fmt::format("animations data '{}' was not loaded", ac.animationsDataTag));
            }
            ac.animationsData = &it->second;

            loader.getIfExists("default_animation", ac.defaultAnimationName);
        });
}

}
