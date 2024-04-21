#pragma once

#include <string>

namespace util
{
// Usage: getDemangledTypename(typeid(something).name())
std::string getDemangledTypename(const char* typeName);
}
