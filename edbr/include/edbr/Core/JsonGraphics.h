#pragma once

#include <nlohmann/json_fwd.hpp>

#include <edbr/Graphics/Color.h>

void from_json(const nlohmann::json& j, LinearColor& c);
