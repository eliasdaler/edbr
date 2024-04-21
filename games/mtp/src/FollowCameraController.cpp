#include "FollowCameraController.h"

#include <edbr/ActionList/ActionWrappers.h>
#include <edbr/DevTools/Im3dState.h>
#include <edbr/Graphics/Camera.h>
#include <edbr/Math/Util.h>
#include <edbr/Util/CameraUtil.h>
#include <edbr/Util/Im3dUtil.h>

#include "Components.h"
#include "PhysicsSystem.h"

#include <im3d.h>
#include <imgui.h>

#include <edbr/Input/InputManager.h>
#include <edbr/Util/InputUtil.h>

FollowCameraController::FollowCameraController(PhysicsSystem& physicsSystem) :
    physicsSystem(physicsSystem)
{}

void FollowCameraController::init(Camera& camera)
{
    camera.setYawPitch(defaultYaw, defaultPitch);
    currentYaw = defaultYaw;
    currentPitch = defaultPitch;
}

void FollowCameraController::teleportCameraToDesiredPosition(Camera& camera)
{
    if (followEntity.entity() == entt::null) {
        return;
    }

    const auto targetPos = util::getOrbitCameraDesiredPosition(
        camera,
        followEntity.get<TransformComponent>().transform,
        cameraYOffset,
        cameraZOffset,
        orbitDistance);
    camera.setPosition(targetPos);

    cameraCurrentTrackPointPos = util::getOrbitCameraTarget(
        followEntity.get<TransformComponent>().transform, cameraYOffset, cameraZOffset);
}

void FollowCameraController::startFollowingEntity(
    entt::handle e,
    Camera& camera,
    bool instantTeleport)
{
    followEntity = e;
    if (instantTeleport) {
        teleportCameraToDesiredPosition(camera);
    }
}

void FollowCameraController::handleInput(const InputManager& im, const Camera& camera, float dt)
{
    const auto& actionMapping = im.getActionMapping();
    static const auto rotateXAxis = actionMapping.getActionTagHash("CameraRotateX");
    static const auto rotateYAxis = actionMapping.getActionTagHash("CameraRotateY");
    const auto rotateStickState = util::getStickState(actionMapping, rotateXAxis, rotateYAxis);

    rotationVelocity.x = -rotateStickState.x * rotateYawSpeed;
    rotationVelocity.y = -rotateStickState.y * rotatePitchSpeed;
}

void FollowCameraController::update(Camera& camera, float dt)
{
    if (followEntity.entity() == entt::null) {
        return;
    }

    if (!followEntity.valid()) {
        followEntity = {};
        return;
    }

    // heading
    if (controlRotation) {
        currentYaw += rotationVelocity.x * dt;
        currentPitch += rotationVelocity.y * dt;
        currentPitch = std::clamp(currentPitch, -1.f, 0.132f);
        camera.setYawPitch(currentYaw, currentPitch);
    } else {
        camera.setYawPitch(defaultYaw, defaultPitch);
    }

    const auto& tc = followEntity.get<TransformComponent>();

    bool isPlayer = true;
    float zOffset = 0.f;
    if (smoothSmartCamera && isPlayer) {
        zOffset = calculateSmartZOffset(followEntity, dt);
    }

    prevOffsetZ = zOffset;

    float yOffset = cameraYOffset;

    // Hacks for faster camera follow during long jumps/falls
    static float timeFalling = 0.f;
    static float timeJumping = 0.f;
    if (isPlayer) {
        if (!physicsSystem.isCharacterOnGround()) {
            if (followEntity.get<MovementComponent>().effectiveVelocity.y < 0.f) {
                timeJumping = 0.f;
                timeFalling += dt;
                yOffset = cameraYOffset - std::min(timeFalling / 0.5f, 1.f) * cameraYOffset;
            } else {
                timeJumping += dt;
            }
        } else {
            timeFalling = 0.f;
            timeJumping = 0.f;
        }
    }

    const auto& followEntityTransform = followEntity.get<TransformComponent>().transform;
    const auto orbitTarget = util::getOrbitCameraTarget(followEntityTransform, yOffset, zOffset);

    if (drawCameraTrackPoint) {
        Im3d::PushLayerId(Im3dState::WorldNoDepthLayer);
        Im3d::DrawPoint({orbitTarget.x, orbitTarget.y, orbitTarget.z}, 5.f, Im3d::Color_Green);
        Im3d::PopLayerId();
    }

    float maxSpeed = cameraMaxSpeed;
    // when the character moves at full speed, rotation can be too much
    // so it's better to slow down the camera to catch up slower
    if (zOffset > 0.5 * maxCameraOffsetFactorRun * cameraZOffset) {
        maxSpeed *= 0.75f;
    }

    cameraCurrentTrackPointPos = util::smoothDamp(
        cameraCurrentTrackPointPos, orbitTarget, followCameraVelocity, cameraDelay, dt, maxSpeed);

    const auto targetPos =
        util::getOrbitCameraDesiredPosition(camera, cameraCurrentTrackPointPos, orbitDistance);
    camera.setPosition(targetPos);

    // update target transform
    desiredTransform.setPosition(targetPos);
    desiredTransform.setHeading(camera.getHeading());
}

float FollowCameraController::calculateSmartZOffset(entt::const_handle followEntity, float dt)
{
    bool playerRunning = false;
    { // TODO: make this prettier?
        const auto& mc = followEntity.get<MovementComponent>();
        auto velocity = mc.effectiveVelocity;
        velocity.y = 0.f;
        const auto velMag = glm::length(velocity);
        playerRunning = velMag > 1.5f;
    }

    float desiredOffset = cameraZOffset;
    if (playerRunning) {
        desiredOffset = cameraZOffset * maxCameraOffsetFactorRun;
    }

    if (prevOffsetZ == desiredOffset) {
        return desiredOffset;
    }

    // this prevents noise - we wait for at least desiredOffsetChangeDelay
    // before changing the desired offset
    if (prevDesiredOffset != desiredOffset) {
        float delay = (desiredOffset == cameraZOffset) ? desiredOffsetChangeDelayRecenter :
                                                         desiredOffsetChangeDelayRun;
        if (desiredOffsetChangeTimer < delay) {
            desiredOffsetChangeTimer += dt;
            return prevOffsetZ;
        } else {
            desiredOffsetChangeTimer = 0.f;
        }
    }
    prevDesiredOffset = desiredOffset;

    // move from max offset to default offset in cameraMaxOffsetTime
    float v = (cameraZOffset * maxCameraOffsetFactorRun - cameraZOffset) / cameraMaxOffsetTime;
    float sign = (prevOffsetZ > desiredOffset) ? -1.f : 1.f;
    if (sign == -1.f) {
        // move faster to default position (cameraZOffset)
        // (otherwise the camera will wait until the player matches with the
        // center which looks weird)
        v *= 1.5f;
    }
    return std::
        clamp(prevOffsetZ + sign * v * dt, cameraZOffset, cameraZOffset * maxCameraOffsetFactorRun);
}

Transform FollowCameraController::getDesiredTransform()
{
    return desiredTransform;
}
