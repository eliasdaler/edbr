#include <edbr/Util/StringUtil.h>

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

bool stringContains(const std::string& str, const std::string& substr)
{
    return str.find(substr) != std::string::npos;
}

} // end of namespace util
