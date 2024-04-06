#include <edbr/Util/OSUtil.h>

#include "Game.h"

int main()
{
    util::setCurrentDirToExeDir();

    Game game{};
    game.init({
        .windowWidth = 2400,
        .windowHeight = 1080,
        // .renderWidth = 1280,
        // .renderHeight = 960,
        .renderWidth = 1440,
        .renderHeight = 1080,
        .title = "Project MTP",
    });
    game.run();
    game.cleanup();
}
