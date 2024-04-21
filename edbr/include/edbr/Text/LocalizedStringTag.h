#pragma once

#include <string>

struct LocalizedStringTag {
    std::string tag;

    bool empty() const { return tag.empty(); }
};

using LST = LocalizedStringTag;
