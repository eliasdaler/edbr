#include <edbr/Graphics/ShadowMapping.h>

#include <array>
#include <cmath>

#include <edbr/Graphics/Camera.h>
#include <edbr/Math/GlobalAxes.h>

#include <glm/gtc/quaternion.hpp>

Camera calculateCSMCamera(
    const std::array<glm::vec3, 8>& frustumCorners,
    const glm::vec3& lightDir,
    float shadowMapSize)
{
    // Refer to https://alextardif.com/shadowmapping.html and
    // https://github.com/wessles/vkmerc/blob/master/base/util/CascadedShadowmap.hpp
    // for details

    // radius of a bounding sphere which will contain the frustum
    // this approach doesn't really work well...
    /* float radius = glm::length(frustumCorners[6] - frustumCorners[0]) / 2.f;
    // find center of frustum (in world coordinates)
    glm::vec3 center{};
    for (const auto& c : frustumCorners) {
        center += c;
    }
    center /= static_cast<float>(frustumCorners.size()); */

    // calculate AABB which contains the frustum
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto& v : frustumCorners) {
        minX = std::min(minX, v.x);
        maxX = std::max(maxX, v.x);
        minY = std::min(minY, v.y);
        maxY = std::max(maxY, v.y);
        minZ = std::min(minZ, v.z);
        maxZ = std::max(maxZ, v.z);
    }

    // radius of sphere that surrounds the AABB
    auto radius = glm::length(glm::vec3{minX, minY, minZ} - glm::vec3{maxX, maxY, maxZ}) / 2.f;
    radius = std::round(radius); // this stabilizes the image somewhat
    // center of AABB
    auto center = glm::mix(glm::vec3{minX, minY, minZ}, glm::vec3{maxX, maxY, maxZ}, 0.5);

    // go to light space and snap to texels
    const auto view = glm::lookAt({}, lightDir, math::GlobalUpAxis);
    auto frustumCenter = glm::vec3{view * glm::vec4{center, 1.f}};
    // round to texel for stabilization
    float texelsPerUnit = (radius * 2.f) / shadowMapSize;
    frustumCenter.x -= std::fmod(frustumCenter.x, texelsPerUnit);
    frustumCenter.y -= std::fmod(frustumCenter.y, texelsPerUnit);

    // go back to world space
    frustumCenter = glm::vec3{glm::inverse(view) * glm::vec4{frustumCenter, 1.f}};

    Camera camera;
    camera.setPosition(frustumCenter);
    camera.setHeading(glm::quatLookAt(lightDir, math::GlobalUpAxis));
    camera.setUseInverseDepth(true);
    float extraScale = 1.5f; // so that we don't cull the objects behind the camera
    camera.initOrtho(radius, radius, -radius * extraScale, radius * extraScale);

    return camera;
}
