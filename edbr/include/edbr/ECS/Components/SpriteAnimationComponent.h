#pragma once

#include <edbr/Graphics/SpriteAnimationData.h>
#include <edbr/Graphics/SpriteAnimator.h>

struct SpriteAnimationComponent {
    SpriteAnimator animator;

    std::string defaultAnimationName{"idle"};
    SpriteAnimationData* animationsData{nullptr};
    std::string animationsDataTag;
};
