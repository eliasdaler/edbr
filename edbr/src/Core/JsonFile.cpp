#include <edbr/Core/JsonFile.h>

#include <fstream>

JsonFile::JsonFile()
{
    data = nlohmann::json{};
}

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

JsonFile::JsonFile(nlohmann::json data) : data(std::move(data))
{}

JsonDataLoader JsonFile::getLoader() const
{
    assert(good);
    return JsonDataLoader(data, path.string());
}

nlohmann::json& JsonFile::getRawData()
{
    assert(good);
    return data;
}

const nlohmann::json& JsonFile::getRawData() const
{
    assert(good);
    return data;
}
