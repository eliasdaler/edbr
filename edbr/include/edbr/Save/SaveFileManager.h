#pragma once

#include <cassert>
#include <filesystem>
#include <memory>
#include <vector>

#include <edbr/Save/SaveFile.h>

class SaveFileManager {
public:
    struct SaveFileInfo {
        std::filesystem::path path;
        std::unique_ptr<SaveFile> file;
        bool exists{false};
    };

    std::filesystem::path findSaveFileDir() const;

    // registerSaveFiles - registers saveFileCount save files in directory saveFileDir.
    // The caller is responsible for making sure that saveFileDir exists.
    // Each save file gets a default path like this: <saveFileDir>/<index>.json
    template<typename SaveFileType, typename... Args>
    void registerSaveFiles(
        std::size_t saveFileCount,
        const std::filesystem::path& saveFileDir,
        Args&&... saveFileCtorArgs)
    {
        assert(saveFiles.empty() && "registerSaveFiles called twice");
        assert(std::filesystem::exists(saveFileDir) && "saveFileDir doesn't exist");
        saveFiles.resize(saveFileCount);
        for (std::size_t i = 0; i < saveFileCount; ++i) {
            auto path = saveFileDir / (std::to_string(i) + ".json");
            auto& si = saveFiles[i];
            si.path = path;
            si.file = std::make_unique<SaveFileType>(std::forward<Args>(saveFileCtorArgs)...);
            si.exists = std::filesystem::exists(si.path);
        }
    }

    // registerSaveFile must be called before calling load/write functions
    SaveFile& loadSaveFile(std::size_t index);
    void writeSaveFile(std::size_t index);

    // Used to see if the save file exists. Usefule for showing <EMPTY> in
    // save file dialogs, for example.
    bool saveFileExists(std::size_t index) const;

    // Gets loaded save file to use in runtime (to get/set data)
    SaveFile& getSaveFile(std::size_t index);

    void setCurrentSaveIndex(std::size_t index);
    SaveFile& getCurrentSaveFile();
    void loadCurrentSaveFile();
    void writeCurrentSaveFile();

    std::size_t getCurrentSaveFileIndex() const { return currentSaveIndex; }
    const std::filesystem::path& getCurrentSaveFilePath() const
    {
        return saveFiles.at(currentSaveIndex).path;
    }

private:
    std::vector<SaveFileInfo> saveFiles;
    std::size_t currentSaveIndex{0}; // index of the save file player is playing
};
