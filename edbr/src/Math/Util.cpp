#include <edbr/Math/Util.h>

#include <array>

#include <glm/gtx/norm.hpp> // length2, dot

namespace util
{
// Adapted from "Foundations of Game Engine Development" Part 2, 9.3 "Bounding Volumes"
math::Sphere calculateBoundingSphere(std::span<glm::vec3> positions)
{
    assert(!positions.empty());
    auto calculateInitialSphere = [](const std::span<glm::vec3>& positions) -> math::Sphere {
        constexpr int dirCount = 13;
        static const std::array<glm::vec3, dirCount> direction = {{
            {1.f, 0.f, 0.f},
            {0.f, 1.f, 0.f},
            {0.f, 0.f, 1.f},
            {1.f, 1.f, 0.f},
            {1.f, 0.f, 1.f},
            {0.f, 1.f, 1.f},
            {1.f, -1.f, 0.f},
            {1.f, 0.f, -1.f},
            {0.f, 1.f, -1.f},
            {1.f, 1.f, 1.f},
            {1.f, -1.f, 1.f},
            {1.f, 1.f, -1.f},
            {1.f, -1.f, -1.f},
        }};

        std::array<float, dirCount> dmin{};
        std::array<float, dirCount> dmax{};
        std::array<std::size_t, dirCount> imin{};
        std::array<std::size_t, dirCount> imax{};

        // Find min and max dot products for each direction and record vertex indices.
        for (int j = 0; j < dirCount; ++j) {
            const auto& u = direction[j];
            dmin[j] = glm::dot(u, positions[0]);
            dmax[j] = dmin[j];
            for (std::size_t i = 1; i < positions.size(); ++i) {
                const auto d = glm::dot(u, positions[i]);
                if (d < dmin[j]) {
                    dmin[j] = d;
                    imin[j] = i;
                } else if (d > dmax[j]) {
                    dmax[j] = d;
                    imax[j] = i;
                }
            };
        }

        // Find direction for which vertices at min and max extents are furthest apart.
        float d2 = glm::length2(positions[imax[0]] - positions[imin[0]]);
        int k = 0;
        for (int j = 1; j < dirCount; j++) {
            const auto m2 = glm::length2(positions[imax[j]] - positions[imin[j]]);
            if (m2 > d2) {
                d2 = m2;
                k = j;
            }
        }

        const auto center = (positions[imin[k]] + positions[imax[k]]) * 0.5f;
        float radius = sqrt(d2) * 0.5f;
        return {center, radius};
    };

    // Determine initial center and radius.
    auto s = calculateInitialSphere(positions);
    // Make pass through vertices and adjust sphere as necessary.
    for (std::size_t i = 0; i < positions.size(); i++) {
        const auto pv = positions[i] - s.center;
        float m2 = glm::length2(pv);
        if (m2 > s.radius * s.radius) {
            auto q = s.center - (pv * (s.radius / std::sqrt(m2)));
            s.center = (q + positions[i]) * 0.5f;
            s.radius = glm::length(q - s.center);
        }
    }
    return s;
}

glm::vec3 smoothDamp(
    const glm::vec3& current,
    glm::vec3 target,
    glm::vec3& currentVelocity,
    float smoothTime,
    float dt,
    float maxSpeed)
{
    if (glm::length(current - target) < 0.0001f) {
        // really close - just return target and stop moving
        currentVelocity = glm::vec3{};
        return target;
    }

    // default smoothing if maxSpeed not passed
    smoothTime = std::max(0.0001f, smoothTime);

    auto change = current - target;

    // limit speed if needed
    float maxChange = maxSpeed * smoothTime;
    float changeSqMag = glm::length2(change);
    if (changeSqMag > maxChange * maxChange) {
        const auto mag = std::sqrt(changeSqMag);
        change *= maxChange / mag;
    }

    const auto originalTarget = target;
    target = current - change;

    // Scary math. (critically damped harmonic oscillator.)
    // See Game Programming Gems 4, ch 1.10 for details
    float omega = 2.f / smoothTime;
    float x = omega * dt;
    // approximation of e^x
    float exp = 1.f / (1.f + x + 0.48f * x * x + 0.235f * x * x * x);
    const auto temp = (currentVelocity + change * omega) * dt;

    currentVelocity = (currentVelocity - temp * omega) * exp;
    glm::vec3 result = target + (change + temp) * exp;
    // prevent overshoot
    if (glm::dot(originalTarget - current, result - originalTarget) > 0) {
        result = originalTarget;
        currentVelocity = (result - originalTarget) / dt;
    }

    return result;
}

}
