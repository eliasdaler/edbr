#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <glm/mat4x4.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

#include <edbr/Graphics/GPUMesh.h>
#include <edbr/Graphics/Light.h>
#include <edbr/Graphics/SkeletalAnimation.h>
#include <edbr/Graphics/SkeletonAnimator.h>
#include <edbr/Math/Transform.h>
#include <edbr/Text/LocalizedStringTag.h>

#include "VirtualCharacterParams.h"

struct MeshComponent {
    std::vector<MeshId> meshes;
    std::vector<MaterialId> meshMaterials;
    std::vector<Transform> meshTransforms;
    bool castShadow{true};
};

struct ColliderComponent {};

struct SkeletonComponent {
    Skeleton skeleton;
    std::vector<SkinnedMesh> skinnedMeshes;
    SkeletonAnimator skeletonAnimator;

    // pointer to the animations stored in SkeletalAnimationCache
    const std::unordered_map<std::string, SkeletalAnimation>* animations{nullptr};

    int skinId{-1}; // reference to skin id from the glTF scene
};

struct LightComponent {
    Light light;
};

struct TriggerComponent {};

struct PlayerSpawnComponent {};

struct CameraComponent {};

struct PlayerComponent {};

struct InteractComponent {
    enum class Type { None, Interact, Talk, Save };
    Type type{Type::Interact};
};

struct PhysicsComponent {
    enum class Type {
        Static,
        Dynamic,
        Kinematic,
    };
    Type type{Type::Static};

    enum class OriginType {
        BottomPlane,
        Center,
    };
    OriginType originType{OriginType::BottomPlane};

    bool sensor{false};

    enum class BodyType {
        None,
        Sphere,
        AABB,
        Capsule,
        Cylinder,
        ConvexHull,
        TriangleMesh,
        VirtualCharacter,
    };

    BodyType bodyType{BodyType::None};

    struct SphereParams {
        float radius{0.f};
    };

    struct AABBParams {
        glm::vec3 min;
        glm::vec3 max;
    };

    struct CylinderParams {
        float radius{0.f};
        float halfHeight{0.f};
    };

    struct CapsuleParams {
        float radius{0.f};
        float halfHeight{0.f};
    };

    std::variant<
        std::monostate,
        SphereParams,
        AABBParams,
        CapsuleParams,
        CylinderParams,
        VirtualCharacterParams>
        bodyParams;

    JPH::BodyID bodyId{};
};

struct AnimationEventSoundComponent {
    const std::string& getSoundName(const std::string& eventName) const
    {
        if (auto it = eventSounds.find(eventName); it != eventSounds.end()) {
            return it->second;
        }
        static const std::string emptyString{};
        return emptyString;
    }

    // event name -> sound name
    std::unordered_map<std::string, std::string> eventSounds;
};

struct FaceComponent {
    struct FaceData {
        MaterialId materialId{NULL_MATERIAL_ID};
    };

    std::unordered_map<std::string, FaceData> faces;
    std::size_t faceMeshIndex; // index into MeshComponent::meshes for mesh which is the face
    std::string currentFace;
    std::string defaultFace;

    std::filesystem::path facesTexturesDir;
    std::unordered_map<std::string, std::string> facesFilenames;
};

struct BlinkComponent {
    float blinkPeriod;
    float blinkHold;
    std::unordered_map<std::string, std::string> faces; // non-blink face -> blink face

    float timer{0.f};
    bool isBlinking{false};

    std::string blinkFace; // if blinking - current blink face
    std::string nonBlinkFace; // face to return to after blink
};
