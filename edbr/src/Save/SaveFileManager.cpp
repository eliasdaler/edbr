#include <edbr/Save/SaveFileManager.h>

#include <fmt/printf.h>

std::filesystem::path SaveFileManager::findSaveFileDir() const

{
    // TODO: find system specific directory where it's okay to store files
    return {};
}

SaveFile& SaveFileManager::loadSaveFile(std::size_t index)
{
    auto& si = saveFiles.at(index);
    assert(si.file);
    fmt::println("Loading save file from {}", si.path.string());
    si.file->loadFromFile(si.path);
    currentSaveIndex = index;
    return *si.file;
}

void SaveFileManager::writeSaveFile(std::size_t index)
{
    auto& si = saveFiles.at(index);
    assert(si.file);
    fmt::println("Writing save file to {}", si.path.string());
    si.file->writeToFile(si.path);
}

bool SaveFileManager::saveFileExists(std::size_t index) const
{
    return saveFiles.at(index).exists;
}

// Gets loaded save file to use in runtime (to get/set data)
SaveFile& SaveFileManager::getSaveFile(std::size_t index)
{
    assert(index < saveFiles.size());
    auto& ptr = saveFiles[index].file;
    assert(ptr);
    return *ptr;
}

void SaveFileManager::setCurrentSaveIndex(std::size_t index)
{
    currentSaveIndex = index;
}

SaveFile& SaveFileManager::getCurrentSaveFile()
{
    return getSaveFile(currentSaveIndex);
}

void SaveFileManager::loadCurrentSaveFile()
{
    loadSaveFile(currentSaveIndex);
}

void SaveFileManager::writeCurrentSaveFile()
{
    writeSaveFile(currentSaveIndex);
}
