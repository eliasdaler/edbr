#include <edbr/DevTools/EntityTreeView.h>

class CustomEntityTreeView : public EntityTreeView {
public:
    void displayExtraFilters() override;
    bool displayEntityInView(entt::const_handle e, const std::string& label) const override;
    const std::string& getEntityDisplayName(entt::const_handle e) const override;
    glm::vec4 getDisplayColor(entt::const_handle e) const override;

private:
    bool displayStaticGeometry{false};
    bool displayTaggedStaticGeometry{true};
    bool displayLights{true};
    bool displayTriggers{true};
    bool displayColliders{false};
};
