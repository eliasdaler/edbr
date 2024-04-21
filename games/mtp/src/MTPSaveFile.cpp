#include "MTPSaveFile.h"

#include "Game.h"

MTPSaveFile::MTPSaveFile(Game& game) : game(game)
{}

void MTPSaveFile::loadFromFile(const std::filesystem::path& path)
{
    SaveFile::loadFromFile(path);
}

void MTPSaveFile::writeToFile(const std::filesystem::path& path)
{
    setData(levelKey, game.getCurrentLevelName());
    setData(spawnKey, game.getLastSpawnName());

    SaveFile::writeToFile(path);
}

std::string MTPSaveFile::getLevelName() const
{
    return getData<std::string>(levelKey);
}

std::string MTPSaveFile::getSpawnName() const
{
    return getData<std::string>(spawnKey);
}
