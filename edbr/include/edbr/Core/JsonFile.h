#pragma once

#include <filesystem>

#include <nlohmann/json.hpp>

#include <edbr/Core/JsonDataLoader.h>

class JsonFile {
public:
    JsonFile();
    JsonFile(const std::filesystem::path& p);
    JsonFile(nlohmann::json data);

    bool isGood() const { return good; }

    JsonDataLoader getLoader() const;
    nlohmann::json& getRawData();
    const nlohmann::json& getRawData() const;

private:
    nlohmann::json data;
    std::filesystem::path path;
    bool good{true};
};
