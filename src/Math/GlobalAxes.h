#pragma once

#include <glm/vec3.hpp>

// Global axes are set-up in right-handed coordinate system
// If you're standing at (0, 0, 0) and looking towards positive Z, then this is what "left",
// "front", "up" etc. will mean to you.

namespace math
{
inline constexpr glm::vec3 GlobalUpAxis{0.f, 1.f, 0.f};
inline constexpr glm::vec3 GlobalDownAxis{0.f, -1.f, 0.f};

inline constexpr glm::vec3 GlobalFrontAxis{0.f, 0.f, 1.f};
inline constexpr glm::vec3 GlobalBackAxis{0.f, 0.f, -1.f};

inline constexpr glm::vec3 GlobalLeftAxis{1.f, 0.f, 0.f};
inline constexpr glm::vec3 GlobalRightAxis{-1.f, 0.f, 0.f};
}

/*
        ^ +Y
        |
        |
        |
<----(^._.^)----->
-X     /         +X
      /
     v  +Z
*/
