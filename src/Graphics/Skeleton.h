#pragma once

#include <limits>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include <Math/Transform.h>

using JointId = std::uint16_t;
static const JointId NULL_JOINT_ID = std::numeric_limits<JointId>::max();
static const JointId ROOT_JOINT_ID = 0;

struct Joint {
    JointId id{NULL_JOINT_ID};
    Transform localTransform;
};

struct Skeleton {
    struct JointNode {
        std::vector<JointId> children;
    };
    std::vector<JointNode> hierarchy;
    std::vector<glm::mat4> inverseBindMatrices;

    std::vector<Joint> joints;
    std::vector<std::string> jointNames;
};
