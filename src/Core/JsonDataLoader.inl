#include <stdexcept>

#include <fmt/format.h>

template<typename T>
void JsonDataLoader::getIfExists(const std::string& valueName, T& obj) const
{
    if (!hasKey(valueName)) {
        return;
    }
    get(valueName, obj);
}

template<typename T>
void JsonDataLoader::get(const std::string& valueName, T& obj) const
{
    get_impl(valueName, obj);
}

template<typename T>
void JsonDataLoader::get(const std::string& valueName, T& obj, const T& defaultValue) const
{
    get_impl(valueName, obj, &defaultValue);
}

template<typename T>
void JsonDataLoader::get_impl(const std::string& valueName, T& obj, const T* defaultValuePtr) const
{
    static_assert(!std::is_enum_v<T>, "Use getEnumValue instead!");
    if (const auto it = data.find(valueName); it != data.end()) {
        try {
            obj = it.value().get<T>();
        } catch (const std::exception& e) {
            throw std::runtime_error(
                fmt::format("can't get field '{}' from '{}': {}", valueName, name, e.what()));
        }
    } else { // the key is missing
        if (overrideMode) {
            return; // nothing to do
        }

        if (!defaultValuePtr) { // default value wasn't provided
            throw std::runtime_error(fmt::format(
                "can't get field '{}' from '{}': the key doesn't "
                "exist and no default value was provided",
                valueName,
                name));
        }

        obj = *defaultValuePtr;
    }
}

template<typename T>
bool JsonDataLoader::hasKey(const T& key) const
{
    return data.find(key) != data.end();
}

template<typename T>
std::vector<T> JsonDataLoader::asVectorOf() const
{
    std::vector<T> v;
    v.reserve(data.size());
    for (const auto& value : data) {
        v.push_back(value);
    }
    return v;
}
