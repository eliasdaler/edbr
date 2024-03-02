#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <Graphics/Skeleton.h>

struct CPUMesh {
    std::vector<std::uint32_t> indices;

    struct Vertex {
        glm::vec3 position;
        float uv_x;
        glm::vec3 normal;
        float uv_y;
        glm::vec4 tangent;
    };
    // interleaved vertices: temporary
    std::vector<Vertex> vertices;

    struct SkinningData {
        glm::vec<4, std::uint32_t> jointIds;
        glm::vec4 weights;
    };
    std::vector<SkinningData> skinningData;

    bool hasSkeleton{false};

    std::string name;

    glm::vec3 minPos;
    glm::vec3 maxPos;
};
