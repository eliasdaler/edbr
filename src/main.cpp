#include <util/OSUtil.h>

#include <Game.h>

int main()
{
    util::setCurrentDirToExeDir();

    Game game;
    game.init();
    game.run();
    game.cleanup();
}
