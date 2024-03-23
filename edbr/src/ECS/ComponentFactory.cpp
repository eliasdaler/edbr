#include <edbr/ECS/ComponentFactory.h>

bool ComponentFactory::componentRegistered(const std::string& componentName) const
{
    return makers.contains(componentName);
}

void ComponentFactory::makeComponent(
    const std::string& componentName,
    entt::handle e,
    const JsonDataLoader& loader) const
{
    const auto it = makers.find(componentName);
    if (it == makers.end()) {
        throw std::runtime_error(
            fmt::format("Component with name '{}' was not registered", componentName));
    }
    const auto& f = it->second;
    f(e, loader);
}
