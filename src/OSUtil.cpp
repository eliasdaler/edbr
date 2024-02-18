#include "OSUtil.h"

namespace osutil
{
std::filesystem::path getExecutableDir()
{
#ifdef _WIN32
    std::wstring buf;
    buf.resize(MAX_PATH);
    do {
        unsigned int len = GetModuleFileNameW(NULL, &buf[0], static_cast<unsigned int>(buf.size()));
        if (len < buf.size()) {
            buf.resize(len);
            break;
        }

        buf.resize(buf.size() * 2);
    } while (buf.size() < 65536);

    return std::filesystem::path(buf).parent_path();
#else
    if (std::filesystem::exists("/proc/self/exe")) {
        return std::filesystem::read_symlink("/proc/self/exe").parent_path();
    }
    return std::filesystem::path();
#endif
}
} // end of namespace osutil
