#pragma once

#include <edbr/Save/SaveFile.h>

class Game;

class MTPSaveFile : public SaveFile {
public:
    MTPSaveFile(Game& game);

    void loadFromFile(const std::filesystem::path& path) override;
    void writeToFile(const std::filesystem::path& path) override;

    std::string getLevelName() const;
    std::string getSpawnName() const;

private:
    Game& game;
    std::string levelKey{"level"};
    std::string spawnKey{"spawn"};
};
