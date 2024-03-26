#pragma once

namespace math
{
template<typename T>
void to_json(nlohmann::json& j, const math::Rect<T>& obj)
{
    j = {obj.left, obj.top, obj.width, obj.height};
}

template<typename T>
void from_json(const nlohmann::json& j, math::Rect<T>& obj)
{
    assert(j.is_array());
    assert(j.size() == 4);
    obj = {j[0].get<T>(), j[1].get<T>(), j[2].get<T>(), j[3].get<T>()};
}
} // end of namespace math

namespace glm
{
template<typename T>
void to_json(nlohmann::json& j, const glm::vec<2, T>& obj)
{
    j = {obj.x, obj.y};
}

template<typename T>
void from_json(const nlohmann::json& j, glm::vec<2, T>& obj)
{
    assert(j.is_array());
    assert(j.size() == 2);
    obj = {j[0].get<T>(), j[1].get<T>()};
}

template<typename T>
void to_json(nlohmann::json& j, const glm::vec<3, T>& obj)
{
    j = {obj.x, obj.y, obj.z};
}

template<typename T>
void from_json(const nlohmann::json& j, glm::vec<3, T>& obj)
{
    assert(j.is_array());
    assert(j.size() == 3);
    obj = {j[0].get<T>(), j[1].get<T>(), j[2].get<T>()};
}

template<typename T>
void from_json(const nlohmann::json& j, glm::vec<4, T>& obj)
{
    assert(j.is_array());
    if (j.size() == 3) {
        obj = {j[0].get<T>(), j[1].get<T>(), j[2].get<T>(), T(1)};
    } else {
        assert(j.size() == 4);
        obj = {j[0].get<T>(), j[1].get<T>(), j[2].get<T>(), j[3].get<T>()};
    }
}

} // end of namespace glm
