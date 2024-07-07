#include <edbr/Util/OSUtil.h>

#include "Game.h"

int main(int argc, char** argv)
{
    util::setCurrentDirToExeDir();

    Game game{};

    game.defineCLIArgs();
    game.parseCLIArgs(argc, argv);

    game.init({
        .appName = "FPS",
        .sourceSubDirName = "fps",
    });
    game.run();
    game.cleanup();
}
