#pragma once

#include <string>
#include <vector>

#include <edbr/Graphics/Bouncer.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/Sprite.h>
#include <edbr/Math/Transform.h>

#include <edbr/UI/NineSliceElement.h>
#include <edbr/UI/TextLabel.h>

#include "Components.h"

class SpriteRenderer;
class Camera;

class GameUI {
public:
    struct UIContext {
        const Camera& camera;
        glm::ivec2 screenSize;
        glm::vec3 playerPos;
        InteractComponent::Type interactionType{InteractComponent::Type::None};
    };

public:
    void init(GfxDevice& gfxDevice);
    void update(float dt);
    void updateDevTools(float dt);

    void draw(SpriteRenderer& uiRenderer, const UIContext& ctx) const;

private:
    void drawInteractTip(SpriteRenderer& uiRenderer, const UIContext& ctx) const;

    void drawUIElement(
        SpriteRenderer& uiRenderer,
        const ui::Element& element,
        const glm::vec2& parentPos) const;

    Font defaultFont;

    std::vector<std::string> strings;

    Sprite interactTipSprite;
    Sprite talkTipSprite;
    Bouncer interactTipBouncer;

    std::unique_ptr<ui::Element> rootUIElement;
};
