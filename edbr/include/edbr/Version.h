#pragma once

#include <fmt/format.h>

struct Version {
    int major{0};
    int minor{0};
    int patch{0};

    std::string toString(bool addV = true) const
    {
        return fmt::format("{}{}.{}.{}", addV ? "v" : "", major, minor, patch);
    }
};
