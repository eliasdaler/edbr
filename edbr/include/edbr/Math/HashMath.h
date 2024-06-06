// hashes for math types
#pragma once

#include <edbr/Math/HashCombine.h>

#include <glm/vec2.hpp>

namespace math
{
template<typename T>
struct hash {};

// specifier
template<typename T>
struct hash<glm::vec<2, T>> {
    inline size_t operator()(const glm::vec<2, T>& v) const
    {
        size_t seed = 0;
        hash_combine(seed, v.x);
        hash_combine(seed, v.x);
        return seed;
    }
};
}
