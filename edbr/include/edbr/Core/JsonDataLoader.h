#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include <edbr/Core/JsonGraphics.h>
#include <edbr/Core/JsonMath.h>

class JsonDataLoader {
public:
    explicit JsonDataLoader(const nlohmann::json& data);
    JsonDataLoader(const nlohmann::json& data, std::string name);

    void setOverride(bool b) { overrideMode = b; }
    bool isOverride() const { return overrideMode; }

    JsonDataLoader getLoader(const std::string& key) const;
    JsonDataLoader getLoader(std::size_t i) const;

    const std::string& getName() const { return name; }

    template<typename T>
    void getIfExists(const std::string& valueName, T& obj) const;

    template<typename T>
    void get(const std::string& valueName, T& obj) const;

    template<typename T>
    void get(const std::string& valueName, T& obj, const T& defaultValue) const;

    template<typename T>
    bool hasKey(const T& key) const;

    std::unordered_map<std::string, JsonDataLoader> getKeyValueMap() const;
    std::unordered_map<std::string, std::string> getKeyValueMapString() const;
    std::unordered_map<std::string, int> getKeyValueMapInt() const;
    std::vector<JsonDataLoader> getVector() const;

    template<typename T>
    std::vector<T> asVectorOf() const;

    const nlohmann::json& getJson() const { return data; }

    std::size_t size() const { return data.size(); }

private:
    template<typename T>
    void get_impl(const std::string& valueName, T& obj, const T* defaultValuePtr = nullptr) const;

    const nlohmann::json& data;
    std::string name;
    bool overrideMode;
};

#include "JsonDataLoader.inl"

// fs::path stuff
namespace nlohmann
{
template<>
struct adl_serializer<std::filesystem::path> {
    static void to_json(nlohmann::json& j, const std::filesystem::path& path) { j = path.string(); }

    static void from_json(const nlohmann::json& j, std::filesystem::path& path)
    {
        path = std::filesystem::path(j.get<std::string>());
    }
};
}
