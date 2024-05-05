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
        animLoader.get("frameDuration", anim.frameDuration);
        animLoader.get("origin", anim.origin);
        animations.emplace(animName, std::move(anim));
    }

    const auto& ssLoader = loader.getLoader("spriteSheet");
    for (const auto& frameLoader : ssLoader.getVector()) {
        SpriteSheet::Frame frame;
        frameLoader.get("frameNum", frame.frameNum);
        frameLoader.get("frame", frame.frameRect);
        spriteSheet.frames.push_back(std::move(frame));
    }
}
