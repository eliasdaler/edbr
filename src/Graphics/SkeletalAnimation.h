#pragma once

#include <string>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

struct SkeletalAnimation {
    struct Tracks {
        std::vector<glm::vec3> translations;
        std::vector<glm::quat> rotations;
        std::vector<glm::vec3> scales;
    };

    std::vector<Tracks> tracks; // index = jointId
    float duration{0.f}; // in seconds
    bool looped{true};

    std::string name;
};
