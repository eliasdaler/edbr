#include "JsonDataLoader.h"

using json = nlohmann::json;

JsonDataLoader::JsonDataLoader(const json& data) : data(data), overrideMode(false)
{}

JsonDataLoader::JsonDataLoader(const json& data, std::string name) :
    data(data), name(std::move(name)), overrideMode(false)
{}

JsonDataLoader JsonDataLoader::getLoader(const std::string& key) const
{
    auto it = data.find(key);
    if (it != data.end()) {
        JsonDataLoader l(it.value(), fmt::format("{}.{}", name, key));
        l.setOverride(overrideMode);
        return l;
    }
    throw std::runtime_error(fmt::format("{} doesn't have a key {}", name, key));
}

JsonDataLoader JsonDataLoader::getLoader(std::size_t i) const
{
    if (i < data.size()) {
        JsonDataLoader l(data[i], fmt::format("{}[{}]", name, i));
        l.setOverride(overrideMode);
        return l;
    }
    throw std::runtime_error(fmt::format("Can't get {}-th value from {}: out of bounds", i, name));
}

std::unordered_map<std::string, JsonDataLoader> JsonDataLoader::getKeyValueMap() const
{
    std::unordered_map<std::string, JsonDataLoader> map(data.size());
    for (const auto& [key, value] : data.items()) {
        JsonDataLoader loader(value, fmt::format("{}.{}", name, key));
        loader.setOverride(overrideMode);
        map.emplace(key, std::move(loader));
    }
    return map;
}

std::unordered_map<std::string, std::string> JsonDataLoader::getKeyValueMapString() const
{
    std::unordered_map<std::string, std::string> map(data.size());
    for (const auto& [key, value] : data.items()) {
        map.emplace(key, value);
    }
    return map;
}

std::unordered_map<std::string, int> JsonDataLoader::getKeyValueMapInt() const
{
    std::unordered_map<std::string, int> map(data.size());
    for (const auto& [key, value] : data.items()) {
        map.emplace(key, value);
    }
    return map;
}

std::vector<JsonDataLoader> JsonDataLoader::getVector() const
{
    std::vector<JsonDataLoader> v;
    v.reserve(data.size());
    int i = 0;
    for (auto& value : data) {
        JsonDataLoader loader(value, fmt::format("{}[{}]", name, i));
        loader.setOverride(overrideMode);
        v.push_back(std::move(loader));
        ++i;
    }
    return v;
}
