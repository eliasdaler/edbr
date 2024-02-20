#include <Renderer.h>

#include <util/OSUtil.h>

int main()
{
    util::setCurrentDirToExeDir();

    Renderer renderer;
    renderer.init();
    renderer.run();
    renderer.cleanup();
}
