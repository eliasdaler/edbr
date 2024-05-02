#pragma once

#include <string>

#include <glm/vec2.hpp>
#include <nlohmann/json.hpp>

struct Spawner {
    std::string prefabName;
    std::string tag;
    glm::vec2 position;
    glm::vec2 heading;

    nlohmann::json prefabData;
};
