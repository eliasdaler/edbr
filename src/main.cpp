#include <Renderer.h>

#include <OSUtil.h>

#include <filesystem>

int main()
{
    const auto exeDir = osutil::getExecutableDir();
    std::filesystem::current_path(exeDir);

    Renderer renderer;
    renderer.init();
    renderer.run();
    renderer.cleanup();
}
