#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include <fmt/format.h>

class JsonDataLoader;

class ComponentFactory {
public:
    // Register a component without a JSON loader (useful for empty components)
    template<typename ComponentType>
    void registerComponent(const std::string& componentName)
    {
        const auto [it, inserted] =
            makers.emplace(componentName, [](entt::handle e, const JsonDataLoader& loader) {
                if (!e.all_of<ComponentType>()) {
                    e.emplace<ComponentType>();
                }
            });
        if (!inserted) {
            throw std::runtime_error(
                fmt::format("component with name '{}' was already registered", componentName));
        }
    }

    // Register a component with JSON loader
    // Example:
    //    registerComponentLoader("Movement",
    //         [](entt::handle e, MovementComponent& mc, const JsonDataLoader&) { ... });
    template<typename F>
    void registerComponentLoader(const std::string& componentName, F f)
    {
        static_assert(
            std::is_same_v<entt::nth_argument_t<0u, F>, entt::handle>,
            "the first argument of the lambda should be entt::handle");
        static_assert(
            std::is_same_v<entt::nth_argument_t<2u, F>, const JsonDataLoader&>,
            "the thid argument of the lambda should be const JsonDataLoader&");

        using ComponentType = std::remove_cvref_t<entt::nth_argument_t<1u, F>>;
        const auto [it, inserted] =
            makers.emplace(componentName, [f](entt::handle e, const JsonDataLoader& loader) {
                auto& c = e.get_or_emplace<ComponentType>();
                f(e, c, loader);
            });
        if (!inserted) {
            throw std::runtime_error(
                fmt::format("component with name '{}' was already registered", componentName));
        }
    }

    bool componentRegistered(const std::string& componentName) const;

    void makeComponent(
        const std::string& componentName,
        entt::handle e,
        const JsonDataLoader& loader) const;

private:
    std::unordered_map<std::string, std::function<void(entt::handle, const JsonDataLoader&)>>
        makers;
};
