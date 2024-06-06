#pragma once

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <edbr/ActionList/ActionList.h>
#include <edbr/Camera/FreeCameraController.h>

class Camera;
class PhysicsSystem;
class ActionListManager;

class FollowCameraController : public CameraController {
public:
    FollowCameraController(PhysicsSystem& physicsSystem);
    void init(Camera& camera);

    void teleportCameraToDesiredPosition(Camera& camera);
    void startFollowingEntity(entt::handle e, Camera& camera, bool instantTeleport = true);

    void handleInput(const InputManager& im, const Camera& camera, float dt) override;
    void update(Camera& camera, float dt) override;
    float calculateSmartZOffset(entt::const_handle followEntity, float dt);

    void changeYawPitchSmooth(Camera& camera, float newYaw, float newPitch, float transitionTime);

    Transform getDesiredTransform() override;

    // data
    PhysicsSystem& physicsSystem;

    entt::handle followEntity;

    glm::vec3 followCameraVelocity;
    glm::vec3 cameraCurrentTrackPointPos;
    float cameraZOffset{0.25f};
    float prevOffsetZ{cameraZOffset};
    float cameraMaxOffsetTime{2.f}; // when time reaches maximum offset
    float maxCameraOffsetFactorRun{7.5f};
    float cameraDelay{0.3f};
    bool drawCameraTrackPoint{false};
    bool smoothSmartCamera{false};

    float defaultYaw{glm::radians(-135.f)};
    float defaultPitch{glm::radians(-20.f)};

    float currentYaw{};
    float currentPitch{};
    bool controlRotation{false};

    float orbitDistance{8.5f};
    float cameraMaxSpeed{6.5f};
    float cameraYOffset{1.5f};

    float prevDesiredOffset{cameraZOffset};
    float desiredOffsetChangeTimer{0.25f};
    float desiredOffsetChangeDelayRun{10.f};
    float desiredOffsetChangeDelayRecenter{0.5f};

    Transform desiredTransform;

    float rotateYawSpeed{1.75f};
    float rotatePitchSpeed{1.5f};
    glm::vec2 rotationVelocity{}; // x - yaw, y - pitch
};
