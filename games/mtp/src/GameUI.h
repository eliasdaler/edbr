#pragma once

#include <string>
#include <vector>

#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/Sprite.h>
#include <edbr/Math/Transform.h>

class SpriteRenderer;

class GameUI {
public:
    void init(GfxDevice& gfxDevice);
    void draw(SpriteRenderer& uiRenderer);

    void updateDevTools(float dt);

private:
    Font defaultFont;

    std::vector<std::string> strings;

    Sprite interactipTipSprite;
    Sprite talkTipSprite;
    Sprite kaeruSprite;
    Sprite testSprite;

    Transform kaeruTransform;

    glm::vec2 rectPos{64.f, 64.f};
    glm::vec2 rectSize{32.f, 32.f};
    glm::vec2 rectPivot{0.5f, 0.5f};
    glm::vec2 rectScale{2.f};
    float rectRotation{0.f};
    float borderWidth{4.f};
    bool insetBorder{true};
};
