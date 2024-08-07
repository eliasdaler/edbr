#include <edbr/Util/OSUtil.h>

#include "Game.h"

int main(int argc, char** argv)
{
    util::setCurrentDirToExeDir();

    Game game{};

    game.defineCLIArgs();
    game.parseCLIArgs(argc, argv);

    game.init({
        .appName = "Project MTP",
        .sourceSubDirName = "mtp",
    });
    game.run();
    game.cleanup();
}
