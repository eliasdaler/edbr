#pragma once

#include <cstddef>
#include <limits>

using ActionTagHash = std::size_t;
static constexpr ActionTagHash ACTION_NONE_HASH = std::numeric_limits<ActionTagHash>::max();
