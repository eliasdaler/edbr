#include "StringUtil.h"

namespace util
{
std::string fromCamelCaseToSnakeCase(const std::string& str)
{
    std::string res;
    bool first = true;
    for (const auto& c : str) {
        if (std::isupper(c)) {
            if (first) {
                first = false;
            } else {
                res += "_";
            }
            res += std::tolower(c);
        } else {
            res += c;
        }
    }
    return res;
}

} // end of namespace util
