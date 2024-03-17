#pragma once

#include <cmath>
#include <string>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

enum class LightType {
    None,
    Directional,
    Point,
    Spot,
};

struct Light {
    std::string name;
    LightType type{LightType::None};

    glm::vec4 color;
    float range{0.f};
    float intensity{0.f};

    glm::vec2 scaleOffset; // precomputed for spot lights (for angle attenuation)

    void setConeAngles(float innerConeAngle, float outerConeAngle)
    {
        // See KHR_lights_punctual spec - formulas are taken from it
        scaleOffset.x = 1.f / std::max(0.001f, std::cos(innerConeAngle) - std::cos(outerConeAngle));
        scaleOffset.y = -std::cos(outerConeAngle) * scaleOffset.x;
    }

    int getShaderType() const
    {
        // see light.glsl - should be the same!
        constexpr int TYPE_DIRECTIONAL_LIGHT = 0;
        constexpr int TYPE_POINT_LIGHT = 1;
        constexpr int TYPE_SPOT_LIGHT = 2;
        switch (type) {
        case LightType::Directional:
            return TYPE_DIRECTIONAL_LIGHT;
        case LightType::Point:
            return TYPE_POINT_LIGHT;
        case LightType::Spot:
            return TYPE_SPOT_LIGHT;
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
    glm::vec3 color;
    float intensity;
    glm::vec2 scaleOffset;
    float shadowMapIndex;
    float unused;
};
