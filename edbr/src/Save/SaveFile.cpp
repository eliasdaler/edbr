#include <edbr/Save/SaveFile.h>

#include <fstream>

void SaveFile::loadFromFile(const std::filesystem::path& path)
{
    json = JsonFile(path);
    assert(json.isGood());
}

void SaveFile::writeToFile(const std::filesystem::path& path)
{
    assert(json.isGood());

    std::ofstream f(path);
    assert(f.good());
    f << std::setw(4) << json.getRawData() << std::endl;
}
