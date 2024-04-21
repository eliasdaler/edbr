#pragma once

#include <string>

#include <edbr/Graphics/Bouncer.h>
#include <edbr/Graphics/Sprite.h>

class ActionMapping;
class AudioManager;
class SpriteRenderer;

namespace ui
{
struct Element;
}

struct Cursor {
    void handleInput(const ActionMapping& am, AudioManager& audioManager);
    void update(float dt);
    void draw(SpriteRenderer& spriteRenderer) const;

    Sprite sprite;
    glm::vec2 spriteScale{1.f};
    Bouncer bouncer;
    bool visible{false};

    ui::Element* selection{nullptr};

    std::string moveSound;
    std::string errorSound;
};
