#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <edbr/Text/LocalizedStringTag.h>

class TextManager {
public:
    void loadText(
        const std::string& language,
        const std::filesystem::path& p,
        const std::filesystem::path& fallbackTextPath);

    bool stringLoaded(const LocalizedStringTag& tag) const;
    const std::string& getString(const LocalizedStringTag& tag) const;

    std::unordered_set<std::uint32_t> getUsedCodePoints(bool includeAllASCII = true) const;

private:
    std::unordered_map<std::string, std::string> loadTextMap(const std::filesystem::path& p);

    std::string language;
    std::unordered_map<std::string, std::string> text;
    std::unordered_map<std::string, std::string> fallbackText;
    static const std::string NULL_STRING;
};
