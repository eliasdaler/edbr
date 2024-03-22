#pragma once

#include <filesystem>

#include <nlohmann/json.hpp>

#include <Core/JsonDataLoader.h>

class JsonFile {
public:
    JsonFile(const std::filesystem::path& p);
    bool isGood() const { return good; }

    JsonDataLoader getLoader() const;

private:
    nlohmann::json data;
    std::filesystem::path path;
    bool good{true};
};
