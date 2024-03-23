#include <edbr/Util/OSUtil.h>

#include "Game.h"

int main()
{
    util::setCurrentDirToExeDir();

    Game game{};
    game.init({.screenWidth = 1280, .screenHeight = 960, .title = "Project MTP"});
    game.run();
    game.cleanup();
}
