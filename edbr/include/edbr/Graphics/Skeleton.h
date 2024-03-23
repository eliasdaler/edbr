#pragma once

#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include <edbr/Graphics/IdTypes.h>
#include <edbr/Math/Transform.h>

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
