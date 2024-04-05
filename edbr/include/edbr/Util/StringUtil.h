#pragma once

#include <string>

namespace util
{
std::string fromCamelCaseToSnakeCase(const std::string& str);

// remove this after update to C++23
bool stringContains(const std::string& str, const std::string& substr);

}
