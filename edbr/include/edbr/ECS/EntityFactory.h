#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <entt/entity/fwd.hpp>

#include <edbr/Core/JsonFile.h>
#include <edbr/ECS/ComponentFactory.h>

class EntityFactory {
public:
    using CreateDefaultEntityFuncType = entt::handle(entt::registry&);
    using PostInitEntityFuncType = void(entt::handle);

public:
    void setCreateDefaultEntityFunc(std::function<CreateDefaultEntityFuncType>&& f);
    void setPostInitEntityFunc(std::function<PostInitEntityFuncType>&& f);
    void registerPrefab(std::string prefabName, const std::filesystem::path& path);

    entt::handle createDefaultEntity(
        entt::registry& registry,
        const std::string& sceneNodeName = emptyString,
        bool postInit = true) const;
    entt::handle createEntity(
        entt::registry& registry,
        const std::string& prefabName,
        const std::string& sceneNodeName = emptyString) const;

    bool prefabExists(const std::string& prefabName) const;

    void addMappedPrefabName(const std::string& from, const std::string& to);
    const std::string& getMappedPrefabName(const std::string& prefabName) const;

    ComponentFactory& getComponentFactory() { return componentFactory; }

private:
    static const std::string emptyString;

    JsonDataLoader getPrefabDataLoader(const std::string& prefabName) const;

    std::function<CreateDefaultEntityFuncType> createDefaultEntityFunc;
    std::function<PostInitEntityFuncType> postInitEntityFunc;

    std::unordered_map<std::string, JsonFile> loadedPrefabFiles;

    // some prefabs can have "aliases" and actually create other prefabs
    std::unordered_map<std::string, std::string> prefabNameMapping;

    // if set, this entity will be created when calling createDefaultEntity
    std::string defaultPrefabName;

    ComponentFactory componentFactory;
};
