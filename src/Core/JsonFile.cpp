#include "JsonFile.h"

#include <fstream>

JsonFile::JsonFile(const std::filesystem::path& p)
{
    std::ifstream file(p);
    if (!file.good()) {
        good = false;
        return;
    }
    file >> data;

    path = p;
}

JsonDataLoader JsonFile::getLoader() const
{
    assert(good);
    return JsonDataLoader(data, path.string());
}
