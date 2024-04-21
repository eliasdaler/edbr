#pragma once

#include <filesystem>

#include <edbr/Core/JsonFile.h>

class SaveFile {
public:
    virtual void loadFromFile(const std::filesystem::path& path);
    virtual void writeToFile(const std::filesystem::path& path);

    // getData - gets data from the save file. Nested keys are not supported.
    // Example:
    //      int numBombs = save.getData<int>("numBombs");
    template<typename T>
    T getData(const std::string& key, const T& defaultValue = {}) const
    {
        T v{};
        json.getLoader().get(key, v, defaultValue);
        return v;
    }

    // setData - sets data to save fiel. Nested keys are not supported.
    // Example:
    //      save.setData("numBombs", 5);
    template<typename T>
    void setData(const std::string& key, const T& value)
    {
        json.getRawData()[key] = value;
    }

    const nlohmann::json& getRawData() const { return json.getRawData(); }

protected:
    JsonFile json;
};
