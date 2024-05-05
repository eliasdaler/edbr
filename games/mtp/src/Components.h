#pragma once

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

struct TagComponent {
    friend class Game;

public:
    // call Game::setEntityTag to set the tag
    const std::string& getTag() const { return tag; }

private:
    void setTag(const std::string& t) { tag = t; }

    std::string tag;
};

struct MeshComponent {
    std::vector<MeshId> meshes;
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

struct NPCComponent {
    LocalizedStringTag name;
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
