#include <edbr/Text/TextManager.h>

#include <edbr/Core/JsonFile.h>

#include <unordered_map>
#include <unordered_set>

#include <utf8.h>

const std::string TextManager::NULL_STRING{"<NULL>"};

void TextManager::loadText(
    const std::string& language,
    const std::filesystem::path& p,
    const std::filesystem::path& fallbackTextPath)
{
    this->language = language;

    text = loadTextMap(p);
    if (!fallbackTextPath.empty() && fallbackTextPath != p) {
        fallbackText = loadTextMap(fallbackTextPath);
    }
}

std::unordered_map<std::string, std::string> TextManager::loadTextMap(
    const std::filesystem::path& path)
{
    JsonFile file(path);
    if (!file.isGood()) {
        throw std::runtime_error(
            fmt::format("failed to load input mapping from {}", path.string()));
    }
    const auto loader = file.getLoader();
    return loader.getKeyValueMapString();
}

std::unordered_set<std::uint32_t> TextManager::getUsedCodePoints(bool includeAllASCII) const
{
    std::unordered_set<std::uint32_t> cps;
    if (includeAllASCII) {
        for (std::uint32_t i = 0; i < 255; ++i) {
            cps.insert(i);
        }
    }
    for (const auto& m : {&text, &fallbackText}) {
        for (const auto& [k, v] : *m) {
            auto it = v.begin();
            const auto e = v.end();
            while (it != e) {
                cps.insert(utf8::next(it, e));
            }
        }
    }
    return cps;
}

bool TextManager::stringLoaded(const LocalizedStringTag& tag) const
{
    if (auto it = text.find(tag.tag); it != text.end()) {
        return true;
    }
    if (auto it = fallbackText.find(tag.tag); it != text.end()) {
        return true;
    }
    return false;
}

const std::string& TextManager::getString(const LocalizedStringTag& tag) const
{
    auto it = text.find(tag.tag);
    if (it == text.end()) {
        fmt::println("ERR: text with tag '{}' was not loaded... looking up fallback", tag.tag);
        auto it2 = fallbackText.find(tag.tag);
        if (it2 == fallbackText.end()) {
            fmt::println("ERR: text with tag '{}' was not loaded", tag.tag);
            return NULL_STRING;
        }
        return it2->second;
    }
    return it->second;
}
