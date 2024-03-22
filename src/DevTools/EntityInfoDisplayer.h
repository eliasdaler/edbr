#pragma once

#include <cassert>
#include <functional>
#include <vector>

#include <entt/entt.hpp>

class EntityInfoDisplayer {
public:
    void displayEntityInfo(entt::const_handle e, float dt);

    bool displayerRegistered(const std::string& componentName) const;

    // Register a component displayer
    // Example:
    //    registerDisplayer("Movement",
    //         [](entt::const_handle e, const MovementComponent& mc) { ... });
    template<typename F>
    void registerDisplayer(const std::string& componentName, F f)
    {
        static_assert(
            std::is_same_v<entt::nth_argument_t<0u, F>, entt::const_handle>,
            "the first argument of the lambda should be entt::const_handle");
        assert(!displayerRegistered(componentName) && "displayer was already registered");

        using ComponentType = std::remove_cvref_t<entt::nth_argument_t<1u, F>>;
        componentDisplayers.push_back({
            .componentName = componentName,
            .componentExists = [](entt::const_handle e) { return e.all_of<ComponentType>(); },
            .displayFunc = [f](entt::const_handle e) { f(e, e.get<ComponentType>()); },
        });
    }

    // Register a component displayer for empty struct components
    template<typename ComponentType>
    void registerEmptyDisplayer(
        const std::string& componentName,
        std::function<void(entt::const_handle)> f = nullptr)
    {
        assert(!displayerRegistered(componentName) && "displayer was already registered");
        componentDisplayers.push_back({
            .componentName = componentName,
            .componentExists = [](entt::const_handle e) { return e.all_of<ComponentType>(); },
            .displayFunc = f,
        });
    }

private:
    struct ComponentDisplayer {
        std::string componentName;
        std::function<bool(entt::const_handle)> componentExists;
        std::function<void(entt::const_handle)> displayFunc;
    };
    std::vector<ComponentDisplayer> componentDisplayers;
};
