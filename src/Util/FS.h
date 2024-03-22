#include <filesystem>

namespace util
{
template<typename F>
void foreachFileInDir(const std::filesystem::path& dir, F f)
{
    for (const auto& p : std::filesystem::recursive_directory_iterator(dir)) {
        if (std::filesystem::is_regular_file(p)) {
            const auto path = p.path().lexically_relative(dir);
            f(p.path());
        }
    }
}
} // end of namespace util
