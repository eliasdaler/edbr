#include <edbr/Graphics/SpriteAnimator.h>

#include <cmath>

#include <edbr/Graphics/Sprite.h>
#include <edbr/Graphics/SpriteSheet.h>

void SpriteAnimator::setAnimation(const SpriteAnimation& a, const std::string& animName)
{
    animation = a;
    animationName = animName;

    progress = 0.f;
    currentFrame = 1;
    frameChanged = true;
    playingAnimation = true;
    animationFinished = true;
}

void SpriteAnimator::setCurrentFrameNumber(int fn)
{
    currentFrame = fn;
    frameChanged = true;
    progress = (currentFrame - 1) / static_cast<float>(animation.getFrameCount());
}

int SpriteAnimator::getSpriteSheetFrameNumber() const
{
    return animation.startFrame + (currentFrame - 1);
}

void SpriteAnimator::update(float dt)
{
    if (!playingAnimation) {
        frameChanged = false; // for animations which just stopped
        return;
    }

    if (animation.getDuration() != 0.f) {
        progress += dt / animation.getDuration();
    }

    if (progress >= 1.f) { // played whole animation
        if (animation.looped) {
            progress -= std::floor(progress); // leave the fractal part only
        } else {
            progress = 1.f;
            frameChanged = (currentFrame != animation.getFrameCount());
            setCurrentFrameNumber(animation.getFrameCount()); // last frame
            playingAnimation = false;
            animationFinished = true;
            return;
        }
    }

    // +1 because frames start from 1
    const auto newFrame = static_cast<int>(std::floor(animation.getFrameCount() * progress)) + 1;
    frameChanged = (currentFrame != newFrame);
    currentFrame = newFrame;
}

void SpriteAnimator::animate(Sprite& sprite, const SpriteSheet& spriteSheet) const
{
    const auto fn = getSpriteSheetFrameNumber();
    sprite.setTextureRect(spriteSheet.getFrameRect(fn));
}
