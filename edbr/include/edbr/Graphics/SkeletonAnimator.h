#pragma once

#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include <edbr/Graphics/Skeleton.h>

struct SkeletalAnimation;

class SkeletonAnimator {
public:
    void setAnimation(const Skeleton& skeleton, const SkeletalAnimation& animation);

    void update(const Skeleton& skeleton, float dt);

    const SkeletalAnimation* getAnimation() const { return animation; }
    const std::string& getCurrentAnimationName() const;

    bool isAnimationFinished() const { return animationFinished; }

    float getProgress() const { return time; }

    void setNormalizedProgress(float t);
    float getNormalizedProgress() const;

    const std::vector<glm::mat4>& getJointMatrices() const { return jointMatrices; };

    bool hasFrameChanged() const { return frameChanged; }
    int getCurrentFrame() const { return currentFrame; }

private:
    void calculateJointMatrices(const Skeleton& skeleton);
    void calculateJointMatrix(
        const Skeleton& skeleton,
        JointId jointId,
        const SkeletalAnimation& animation,
        float time,
        const glm::mat4& parentTransform);

    float time{0}; // current animation time (in seconds)
    const SkeletalAnimation* animation{nullptr};
    bool animationFinished{false};

    int currentFrame{0};
    bool firstFrame = true;
    bool frameChanged{false};

    std::vector<glm::mat4> jointMatrices;
};
