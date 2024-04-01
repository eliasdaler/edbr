#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Color.h>

#include <im3d_math.h>

namespace util
{
inline JPH::Vec3 glmToJolt(const glm::vec3& v)
{
    return JPH::Vec3{v.x, v.y, v.z};
}

inline JPH::Float3 glmToJoltFloat3(const glm::vec3& v)
{
    return JPH::Float3{v.x, v.y, v.z};
}

inline JPH::Quat glmToJolt(const glm::quat& q)
{
    return JPH::Quat{q.x, q.y, q.z, q.w};
}

inline glm::vec3 joltToGLM(const JPH::Vec3 v)
{
    return {v.GetX(), v.GetY(), v.GetZ()};
}

inline glm::quat joltToGLM(const JPH::Quat q)
{
    return glm::quat{q.GetW(), q.GetX(), q.GetY(), q.GetZ()};
}

inline Im3d::Color joltToIm3d(const JPH::Color& color)
{
    return Im3d::Color(
        (float)color.r / 255.f,
        (float)color.g / 255.f,
        (float)color.b / 255.f,
        (float)color.a / 255.f);
}

inline Im3d::Vec3 joltToIm3d(const JPH::RVec3& v)
{
    return {
        v.GetX(),
        v.GetY(),
        v.GetZ(),
    };
}

inline Im3d::Vec3 joltToIm3d(const JPH::Float3& v)
{
    return {
        v.x,
        v.y,
        v.z,
    };
}

}
