#include <edbr/Graphics/Scene.h>

namespace edbr
{
math::AABB calculateBoundingBoxLocal(const Scene& scene, const std::vector<MeshId> meshes)
{
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();

    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto& meshId : meshes) {
        auto& mesh = scene.cpuMeshes.at(meshId);
        for (const auto& v : mesh.vertices) {
            minX = std::min(minX, v.position.x);
            minY = std::min(minY, v.position.y);
            minZ = std::min(minZ, v.position.z);

            maxX = std::max(maxX, v.position.x);
            maxY = std::max(maxY, v.position.y);
            maxZ = std::max(maxZ, v.position.z);
        }
    }
    return math::AABB{
        .min = glm::vec3{minX, minY, minZ},
        .max = glm::vec3{maxX, maxY, maxZ},
    };
}
}
