#include <Graphics/SkeletonAnimator.h>

#include <Graphics/SkeletalAnimation.h>
#include <Graphics/Skeleton.h>

#include <tuple>

#include <glm/gtx/compatibility.hpp> // lerp for vec3

namespace
{
static const int ANIMATION_FPS = 30;
static const glm::mat4 I{1.f};
}

void SkeletonAnimator::setAnimation(const Skeleton& skeleton, const SkeletalAnimation& animation)
{
    if (this->animation != nullptr && this->animation->name == animation.name) {
        return; // TODO: allow to reset animation
    }

    jointMatrices.resize(skeleton.joints.size());

    time = 0.f;
    animationFinished = false;
    this->animation = &animation;

    calculateJointMatrices(skeleton);
}

void SkeletonAnimator::update(const Skeleton& skeleton, float dt)
{
    if (!animation || animationFinished) {
        return;
    }

    time += dt;
    if (time > animation->duration) { // loop
        if (animation->looped) {
            time -= animation->duration;
        } else {
            time = animation->duration;
            animationFinished = true;
        }
    }

    calculateJointMatrices(skeleton);
}

const std::string& SkeletonAnimator::getCurrentAnimationName() const
{
    static const std::string nullAnimationName{};
    return animation ? animation->name : nullAnimationName;
}

namespace
{

std::tuple<std::size_t, std::size_t, float> findPrevNextKeys(std::size_t numKeys, float time)
{
    // keys are sampled by ANIMATION_FPS, so finding prev/next key is easy
    const std::size_t prevKey =
        std::min((std::size_t)std::floor(time * ANIMATION_FPS), numKeys - 1);
    const std::size_t nextKey = std::min(prevKey + 1, numKeys - 1);

    float t{0.f};
    if (prevKey != nextKey) {
        t = time * ANIMATION_FPS - (float)prevKey;
    }

    return {prevKey, nextKey, t};
}

glm::mat4 sampleAnimation(const SkeletalAnimation& animation, JointId jointId, float time)
{
    const auto& ts = animation.tracks[jointId];

    auto tm = I;

    { // translation
        const auto& tc = ts.translations;
        if (!tc.empty()) {
            const auto [p, n, t] = findPrevNextKeys(tc.size(), time);
            const auto pos = glm::lerp(tc[p], tc[n], t);
            tm[3] = glm::vec4(pos, 1.f);
        }
    }

    { // rotation
        const auto& rc = ts.rotations;
        if (!rc.empty()) {
            const auto [p, n, t] = findPrevNextKeys(rc.size(), time);
            const auto rot = glm::slerp(rc[p], rc[n], t);
            tm *= glm::mat4_cast(rot);
        }
    }

    // TODO: one more potential optimization: if there's no scaling in
    // animation, we can skip this calculating the scale.

    { // scale
        const auto& sc = ts.scales;
        if (!sc.empty()) {
            const auto [p, n, t] = findPrevNextKeys(sc.size(), time);
            const auto scale = glm::lerp(sc[p], sc[n], t);
            tm = glm::scale(tm, scale);
        }
    }

    return tm;
}

} // end of anonymous namespace

void SkeletonAnimator::calculateJointMatrices(const Skeleton& skeleton)
{
    static const glm::mat4 I{1.f};
    calculateJointMatrix(skeleton, ROOT_JOINT_ID, *animation, time, I);
}

void SkeletonAnimator::calculateJointMatrix(
    const Skeleton& skeleton,
    JointId jointId,
    const SkeletalAnimation& animation,
    float time,
    const glm::mat4& parentTransform)
{
    const auto localTransform = sampleAnimation(animation, jointId, time);
    const auto modelTransform = parentTransform * localTransform;
    jointMatrices[jointId] = modelTransform * skeleton.inverseBindMatrices[jointId];

    for (const auto childIdx : skeleton.hierarchy[jointId].children) {
        calculateJointMatrix(skeleton, childIdx, animation, time, modelTransform);
    }
}

void SkeletonAnimator::setNormalizedProgress(float t)
{
    assert(t >= 0.f && t <= 1.f);
    time = t * animation->duration;
}

float SkeletonAnimator::getNormalizedProgress() const
{
    if (!animation) {
        return 0.f;
    }
    return time / animation->duration;
}
