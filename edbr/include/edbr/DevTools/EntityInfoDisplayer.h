#pragma once

#include <cassert>
#include <functional>
#include <vector>

#include <entt/entity/entity.hpp>
#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

class EntityInfoDisplayer {
public:
    enum class DisplayStyle { Default, CollapsedByDefault };

    void displayEntityInfo(entt::handle e, float dt);

    bool displayerRegistered(const std::string& componentName) const;

    // Register a component displayer
    // Example:
    //    registerDisplayer("Movement",
    //         [](entt::handle e, const MovementComponent& mc) { ... });
    template<typename F>
    void registerDisplayer(
        const std::string& componentName,
        F f,
        DisplayStyle style = DisplayStyle::Default)
    {
        static_assert(
            std::is_same_v<entt::nth_argument_t<0u, F>, entt::handle>,
            "the first argument of the lambda should be entt::handle");
        assert(!displayerRegistered(componentName) && "displayer was already registered");

        using ComponentType = std::remove_cvref_t<entt::nth_argument_t<1u, F>>;
        componentDisplayers.push_back({
            .componentName = componentName,
            .componentExists = [](entt::handle e) { return e.all_of<ComponentType>(); },
            .displayFunc = [f](entt::handle e) { f(e, e.get<ComponentType>()); },
            .style = style,
        });
    }

    // Register a component displayer for empty struct components
    template<typename ComponentType>
    void registerEmptyDisplayer(
        const std::string& componentName,
        std::function<void(entt::handle)> f = nullptr,
        DisplayStyle style = DisplayStyle::Default)
    {
        assert(!displayerRegistered(componentName) && "displayer was already registered");
        componentDisplayers.push_back({
            .componentName = componentName,
            .componentExists = [](entt::handle e) { return e.all_of<ComponentType>(); },
            .displayFunc = f,
            .style = style,
        });
    }

private:
    struct ComponentDisplayer {
        std::string componentName;
        std::function<bool(entt::handle)> componentExists;
        std::function<void(entt::handle)> displayFunc;
        DisplayStyle style;
    };
    std::vector<ComponentDisplayer> componentDisplayers;
};
