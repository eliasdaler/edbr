#include <edbr/Graphics/SpriteAnimationData.h>

#include <edbr/Core/JsonFile.h>

void SpriteAnimationData::load(const std::filesystem::path& path)
{
    JsonFile file(path);
    if (!file.isGood()) {
        throw std::runtime_error(
            fmt::format("failed to load animation data from {}", path.string()));
    }
    const auto loader = file.getLoader();

    const auto& animationsLoader = loader.getLoader("animations");
    for (const auto& [animName, animLoader] : animationsLoader.getKeyValueMap()) {
        SpriteAnimation anim;
        animLoader.get("startFrame", anim.startFrame);
        animLoader.get("endFrame", anim.endFrame);

        int frameDurationMS{};
        animLoader.get("frameDurationMS", frameDurationMS);
        anim.frameDuration = frameDurationMS / 1000.f;

        animLoader.getIfExists("origin", anim.origin);

        animations.emplace(animName, std::move(anim));
    }

    const auto& ssLoader = loader.getLoader("spriteSheet").getLoader("frames");
    spriteSheet.frames = ssLoader.asVectorOf<math::IntRect>();
}
