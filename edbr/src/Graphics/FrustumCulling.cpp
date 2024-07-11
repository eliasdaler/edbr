#include <edbr/Graphics/FrustumCulling.h>

#include <edbr/Graphics/Camera.h>
#include <edbr/Math/AABB.h>
#include <edbr/Math/Sphere.h>

#include <algorithm>
#include <array>

namespace edge
{

std::array<glm::vec3, 8> calculateFrustumCornersWorldSpace(const Camera& camera)
{
    const auto nearDepth = camera.usesInverseDepth() ? 1.0f : 0.f;
    const auto farDepth = camera.usesInverseDepth() ? 0.0f : 1.f;
    const auto bottomY = camera.isClipSpaceYDown() ? 1.f : -1.f;
    const auto topY = camera.isClipSpaceYDown() ? -1.f : 1.f;
    const std::array<glm::vec3, 8> cornersNDC = {
        // near plane
        glm::vec3{-1.f, bottomY, nearDepth},
        glm::vec3{-1.f, topY, nearDepth},
        glm::vec3{1.f, topY, nearDepth},
        glm::vec3{1.f, bottomY, nearDepth},
        // far plane
        glm::vec3{-1.f, bottomY, farDepth},
        glm::vec3{-1.f, topY, farDepth},
        glm::vec3{1.f, topY, farDepth},
        glm::vec3{1.f, bottomY, farDepth},
    };

    const auto inv = glm::inverse(camera.getViewProj());
    std::array<glm::vec3, 8> corners{};
    for (int i = 0; i < 8; ++i) {
        auto corner = inv * glm::vec4(cornersNDC[i], 1.f);
        corner /= corner.w;
        corners[i] = glm::vec3{corner};
    }
    return corners;
}

Frustum createFrustumFromCamera(const Camera& camera)
{
    // http://www8.cs.umu.se/kurser/5DV051/HT12/lab/plane_extraction.pdf
    // Need to negate everything because we're looking at -Z, not +Z
    const auto m = camera.getViewProj();

    Frustum frustum;
    /*
    // if clipNearZ == -1
    frustum.nearFace =
        {-(m[0][3] + m[0][2]), -(m[1][3] + m[1][2]), -(m[2][3] + m[2][2]), -(m[3][3] + m[3][2])};
    */
    // clipNearZ == 0
    frustum.nearFace = {-(m[0][2]), -(m[1][2]), -(m[2][2]), -(m[3][2])};

    frustum.farFace =
        {-(m[0][3] - m[0][2]), -(m[1][3] - m[1][2]), -(m[2][3] - m[2][2]), -(m[3][3] - m[3][2])};

    frustum.leftFace =
        {-(m[0][3] + m[0][0]), -(m[1][3] + m[1][0]), -(m[2][3] + m[2][0]), -(m[3][3] + m[3][0])};
    frustum.rightFace =
        {-(m[0][3] - m[0][0]), -(m[1][3] - m[1][0]), -(m[2][3] - m[2][0]), -(m[3][3] - m[3][0])};

    frustum.bottomFace =
        {-(m[0][3] + m[0][1]), -(m[1][3] + m[1][1]), -(m[2][3] + m[2][1]), -(m[3][3] + m[3][1])};
    frustum.topFace =
        {-(m[0][3] - m[0][1]), -(m[1][3] - m[1][1]), -(m[2][3] - m[2][1]), -(m[3][3] - m[3][1])};
    return frustum;
}

namespace
{
    bool isOnOrForwardPlane(const Frustum::Plane& plane, const math::Sphere& sphere)
    {
        return plane.getSignedDistanceToPlane(sphere.center) > -sphere.radius;
    }

    glm::vec3 getTransformScale(const glm::mat4& transform)
    {
        float sx = glm::length(glm::vec3{transform[0][0], transform[0][1], transform[0][2]});
        float sy = glm::length(glm::vec3{transform[1][0], transform[1][1], transform[1][2]});
        float sz = glm::length(glm::vec3{transform[2][0], transform[2][1], transform[2][2]});
        return {sx, sy, sz};
    }
} // end of anonymous namespace

bool isInFrustum(const Frustum& frustum, const math::Sphere& s)
{
    bool res = true;
    for (int i = 0; i < 6; ++i) {
        const auto& plane = frustum.getPlane(i);
        const auto dist = plane.getSignedDistanceToPlane(s.center);
        if (dist > s.radius) {
            return false;
        } else if (dist > -s.radius) {
            res = true;
        }
    }
    return res;
}

bool isInFrustum(const Frustum& frustum, const math::AABB& aabb)
{
    bool ret = true;
    for (int i = 0; i < 6; ++i) {
        const auto& plane = frustum.getPlane(i);

        // Nearest point
        glm::vec3 p;
        p.x = plane.normal.x >= 0 ? aabb.min.x : aabb.max.x;
        p.y = plane.normal.y >= 0 ? aabb.min.y : aabb.max.y;
        p.z = plane.normal.z >= 0 ? aabb.min.z : aabb.max.z;
        if (plane.getSignedDistanceToPlane(p) > 0) {
            return false;
        }

        // Farthest point
        glm::vec3 f;
        f.x = plane.normal.x >= 0 ? aabb.max.x : aabb.min.x;
        f.y = plane.normal.y >= 0 ? aabb.max.y : aabb.min.y;
        f.z = plane.normal.z >= 0 ? aabb.max.z : aabb.min.z;
        if (plane.getSignedDistanceToPlane(f) > 0) {
            ret = true;
        }
    }
    return ret;
}

math::Sphere calculateBoundingSphereWorld(
    const glm::mat4& transform,
    const math::Sphere& s,
    bool hasSkeleton)
{
    const auto scale = getTransformScale(transform);
    float maxScale = std::max({scale.x, scale.y, scale.z});
    if (hasSkeleton) {
        maxScale = 5.f; // ignore scale for skeleton meshes (TODO: fix)
                        // setting scale to 1.f causes prolems with frustum culling
    }
    auto sphereWorld = s;
    sphereWorld.radius *= maxScale;
    sphereWorld.center = glm::vec3(transform * glm::vec4(sphereWorld.center, 1.f));
    return sphereWorld;
}

} // end of namespace edge
