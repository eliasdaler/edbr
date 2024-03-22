#pragma once

#include <glm/ext/quaternion_float.hpp>
#include <glm/vec2.hpp>

#include <nlohmann/json.hpp>

#include <Math/Rect.h>

namespace math
{
template<typename T>
void to_json(nlohmann::json& j, const math::Rect<T>& obj);

template<typename T>
void from_json(const nlohmann::json& j, math::Rect<T>& obj);

}

namespace glm
{
// TODO: move to JsonGLM.h
template<typename T>
void to_json(nlohmann::json& j, const glm::vec<2, T>& obj);

template<typename T>
void from_json(const nlohmann::json& j, glm::vec<2, T>& obj);

template<typename T>
void to_json(nlohmann::json& j, const glm::vec<3, T>& obj);

template<typename T>
void from_json(const nlohmann::json& j, glm::vec<3, T>& obj);

template<typename T>
void from_json(const nlohmann::json& j, glm::vec<4, T>& obj);

void to_json(nlohmann::json& j, const glm::quat& obj);
void from_json(const nlohmann::json& j, glm::quat& obj);
}

#include "JsonMath.inl"
