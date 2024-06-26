#pragma once

#include <cmath>
#include <string>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <edbr/Graphics/Color.h>
#include <edbr/Graphics/IdTypes.h>

enum class LightType {
    None,
    Directional,
    Point,
    Spot,
};

namespace edbr
{
inline constexpr int TYPE_DIRECTIONAL_LIGHT = 0;
inline constexpr int TYPE_POINT_LIGHT = 1;
inline constexpr int TYPE_SPOT_LIGHT = 2;
}

struct Light {
    std::string name;
    LightType type{LightType::None};

    LinearColor color;
    float range{0.f};
    float intensity{0.f};

    glm::vec2 scaleOffset; // precomputed for spot lights (for angle attenuation)

    void setConeAngles(float innerConeAngle, float outerConeAngle)
    {
        // See KHR_lights_punctual spec - formulas are taken from it
        scaleOffset.x = 1.f / std::max(0.001f, std::cos(innerConeAngle) - std::cos(outerConeAngle));
        scaleOffset.y = -std::cos(outerConeAngle) * scaleOffset.x;
    }

    int getGPUType() const
    {
        // see light.glsl - should be the same!
        switch (type) {
        case LightType::Directional:
            return edbr::TYPE_DIRECTIONAL_LIGHT;
        case LightType::Point:
            return edbr::TYPE_POINT_LIGHT;
        case LightType::Spot:
            return edbr::TYPE_SPOT_LIGHT;
        default:
            assert(false);
        }
        return 0;
    }

    bool castShadow{true};
};

// Representation of light data on GPU (see lighting.glsl)
struct GPULightData {
    glm::vec3 position;
    std::uint32_t type;
    glm::vec3 direction;
    float range;
    LinearColorNoAlpha color;
    float intensity;
    glm::vec2 scaleOffset;
    ImageId shadowMapID;
    float unused;
};
