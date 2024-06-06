#include <edbr/ECS/EntityFactory.h>

#include <iostream>

#include <edbr/ECS/Components/MetaInfoComponent.h>

namespace
{
nlohmann::json mergeJson(const nlohmann::json& a, const nlohmann::json& b)
{
    auto result = a.flatten();
    const auto tmp = b.flatten();
    for (const auto& [k, v] : tmp.items()) {
        result[k] = v;
    }
    return result.unflatten();
}
}

const std::string EntityFactory::emptyString{};

void EntityFactory::setCreateDefaultEntityFunc(std::function<CreateDefaultEntityFuncType>&& f)
{
    createDefaultEntityFunc = f;
}

void EntityFactory::setPostInitEntityFunc(std::function<PostInitEntityFuncType>&& f)
{
    postInitEntityFunc = f;
}

void EntityFactory::registerPrefab(std::string prefabName, const std::filesystem::path& path)
{
    JsonFile file(path);
    assert(file.isGood());
    addPrefabFile(std::move(prefabName), std::move(file));
}

void EntityFactory::addPrefabFile(std::string prefabName, JsonFile file)
{
    assert(file.isGood());
    auto [it, inserted] = loadedPrefabFiles.emplace(std::move(prefabName), std::move(file));
    if (!inserted) {
        throw std::runtime_error(
            fmt::format("Prefab with name '{}' was already registered", prefabName));
    }
}

entt::handle EntityFactory::createDefaultEntity(entt::registry& registry, bool postInit) const
{
    auto e = createDefaultEntityFunc ? createDefaultEntityFunc(registry) :
                                       entt::handle{registry, registry.create()};
    if (!e.all_of<MetaInfoComponent>()) {
        e.emplace<MetaInfoComponent>();
    }
    if (postInit && postInitEntityFunc) {
        postInitEntityFunc(e);
    }
    return e;
}

entt::handle EntityFactory::createEntity(
    entt::registry& registry,
    const std::string& prefabName,
    const nlohmann::json& overridePrefabData) const
{
    const auto& mappedPrefabName = getMappedPrefabName(prefabName);
    const auto& actualPrefabName = !mappedPrefabName.empty() ? mappedPrefabName : prefabName;
    const auto prefabLoader = getPrefabDataLoader(actualPrefabName);

    // merge override data (if present)
    if (!overridePrefabData.empty()) {
        const auto& originalData = prefabLoader.getJson();
        const auto jsonData = mergeJson(originalData, overridePrefabData);
        JsonDataLoader loader{jsonData, prefabLoader.getName() + "(+ overload data)"};
        return createEntity(registry, actualPrefabName, loader);
    }

    return createEntity(registry, actualPrefabName, prefabLoader);
}

entt::handle EntityFactory::createEntity(
    entt::registry& registry,
    const std::string& prefabName,
    const JsonDataLoader& prefabLoader) const
{
    auto e = createDefaultEntity(registry, false);
    for (const auto& [componentName, loader] : prefabLoader.getKeyValueMap()) {
        if (!componentFactory.componentRegistered(componentName)) {
            std::cout << "prefabName=" << prefabName << ": component '" << componentName
                      << "' was not registered. Skipping..." << std::endl;
            continue;
        }
        componentFactory.makeComponent(componentName, e, loader);
    }

    e.get<MetaInfoComponent>().prefabName = prefabName;

    if (postInitEntityFunc) {
        postInitEntityFunc(e);
    }

    return e;
}

JsonDataLoader EntityFactory::getPrefabDataLoader(const std::string& prefabName) const
{
    auto it = loadedPrefabFiles.find(prefabName);
    if (it == loadedPrefabFiles.end()) {
        throw std::runtime_error(
            fmt::format("Prefab with name '{}' was not registered", prefabName));
    }
    return it->second.getLoader();
}

bool EntityFactory::prefabExists(const std::string& prefabName) const
{
    return loadedPrefabFiles.contains(prefabName);
}

void EntityFactory::addMappedPrefabName(const std::string& from, const std::string& to)
{
    if (from == to) {
        assert(false && "huh?");
        return;
    }

    if (prefabNameMapping.contains(from)) {
        throw std::runtime_error(
            fmt::format("Mapping for prefab '{}' was already added: {}", from, to));
    }
    if (prefabExists(to)) {
        prefabNameMapping.emplace(from, to);
        return;
    }

    auto mapped = getMappedPrefabName(to);
    if (!mapped.empty()) {
        prefabNameMapping.emplace(from, mapped);
        return;
    }
    throw std::runtime_error(fmt::format("Prefab with name '{}' was not registered", to));
}

const std::string& EntityFactory::getMappedPrefabName(const std::string& prefabName) const
{
    if (const auto it = prefabNameMapping.find(prefabName); it != prefabNameMapping.end()) {
        return it->second;
    }
    if (prefabExists(prefabName)) {
        return prefabName;
    }

    static const std::string emptyString{};
    return emptyString;
}
