#pragma once

#include <string>

#include <edbr/Graphics/SpriteAnimation.h>

class Sprite;
struct SpriteSheet;

class SpriteAnimator {
public:
    void update(float dt);

    void setAnimation(const SpriteAnimation& a, const std::string& animName);
    void setCurrentFrameNumber(int fn);
    int getSpriteSheetFrameNumber() const;

    void animate(Sprite& sprite, const SpriteSheet& spriteSheet) const;

    const std::string& getAnimationName() const { return animationName; }
    const SpriteAnimation& getAnimation() const { return animation; }
    int getCurrentFrame() const { return currentFrame; }
    float getProgress() const { return progress; }

private:
    SpriteAnimation animation;
    std::string animationName;
    // direction?

    float progress{0.f}; // progress from 0 to 1 (e.g. 0.5 is a halfway of animation)
    int currentFrame{1}; // from 1 to len(animation)
    bool playingAnimation{false};
    bool frameChanged{false}; // true if frame changed this frame
    bool animationFinished{false}; // becomes true if non-looped animation has finished
};
