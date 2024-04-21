#include "Game.h"

#include "Levels/CityLevelScript.h"
#include "Levels/HouseLevelScript.h"

void Game::registerLevels()
{
    registerLevelScript<CityLevelScript>("city");
    registerLevelScript<HouseLevelScript>("house");
}
