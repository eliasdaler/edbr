#include "GameUI.h"

#include <Graphics/BaseRenderer.h>
#include <Graphics/SpriteRenderer.h>

#include <random>

#include <utf8.h>

namespace
{

std::unordered_set<std::uint32_t> getUsedCodePoints(
    const std::vector<std::string>& strings,
    bool includeAllASCII)
{
    std::unordered_set<std::uint32_t> cps;
    if (includeAllASCII) {
        for (std::uint32_t i = 0; i < 255; ++i) {
            cps.insert(i);
        }
    }
    for (const auto& s : strings) {
        auto it = s.begin();
        const auto e = s.end();
        while (it != e) {
            cps.insert(utf8::next(it, e));
        }
    }
    return cps;
}
}

void GameUI::init(BaseRenderer& renderer)
{
    strings = {
        "テキストのレンダリング、すごい",
        "Text rendering works!",
        "Multiline text\nworks too",
    };

    auto neededCodePoints = getUsedCodePoints(strings, true);

    bool ok = defaultFont.load(
        renderer.getGfxDevice(),
        renderer,
        "assets/fonts/JF-Dot-Kappa20B.ttf",
        32,
        neededCodePoints);

    assert(ok && "font failed to load");

    const auto interactTipImageId = renderer.loadImageFromFile("assets/images/ui/interact_tip.png");
    const auto& interactTipImage = renderer.getImage(interactTipImageId);
    interactipTipSprite.setTexture(interactTipImage);
    interactipTipSprite.pivot = glm::vec2{0.5f, 1.2f};

    const auto talkTipImageId = renderer.loadImageFromFile("assets/images/ui/talk_tip.png");
    const auto& talkTipImage = renderer.getImage(talkTipImageId);
    talkTipSprite.setTexture(talkTipImage);

    const auto& kaeruImageId = renderer.loadImageFromFile("assets/images/kaeru.png");
    const auto& kaeruImage = renderer.getImage(kaeruImageId);
    kaeruSprite.setTexture(kaeruImage);
    kaeruSprite.setTextureRect({48, 64, 16, 16});

    const auto& testImageId = renderer.loadImageFromFile("assets/images/test_sprite.png");
    const auto& testImage = renderer.getImage(testImageId);
    testSprite.setTexture(testImage);
    testSprite.pivot = glm::vec2{0.5f, 0.5f};

    kaeruTransform.setPosition({240.f, 160.f, 0.f});
    kaeruTransform.setScale(glm::vec3{4.f, 4.f, 1.f});
}

void GameUI::draw(SpriteRenderer& uiRenderer)
{
#if 1
    { // draw test UI
        // uiRenderer.drawSprite(talkTipSprite, {128.f, 160.f});

        /*
        uiRenderer.drawSprite(testSprite, {128.f, 250.f}, glm::pi<float>() / 4.f);
        uiRenderer.drawFilledRect(
            {128.f, 250.f, 64.f, 64.f},
            glm::vec4{1.f, 1.f, 0.f, 0.5f},
            glm::pi<float>() / 4.f,
            glm::vec2{1.f},
            glm::vec2{0.5f, 0.5f});
        uiRenderer.drawFilledRect(
            math::FloatRect{0.f, 0.f, 128.f, 250.f}, glm::vec4{0.f, 0.f, 1.f, 0.5f}); */

        // uiRenderer.drawSprite(kaeruSprite, kaeruTransform.asMatrix());

        uiRenderer.drawText(defaultFont, strings[0], glm::vec2{64.f, 64.f});
        uiRenderer.drawText(
            defaultFont, strings[1], glm::vec2{64.f, 90.f}, glm::vec4{1.f, 1.f, 0.f, 1.f});

        uiRenderer.drawText(
            defaultFont, strings[2], glm::vec2{64.f, 120.f}, glm::vec4{1.f, 0.f, 1.f, 1.f});

        /* for (int i = 0; i < 8; ++i) {
            const auto rotation = i * glm::pi<float>() / 4.f;
            uiRenderer.drawSprite(interactipTipSprite, glm::vec2{64.f, 100.f}, rotation);
        }

        uiRenderer.drawFilledRect(
            {rectPos, rectSize},
            glm::vec4{1.f, 1.f, 0.f, 0.5f},
            rectRotation,
            rectScale,
            rectPivot);
        uiRenderer.drawRect(
            {rectPos, rectSize},
            glm::vec4{1.f, 0.f, 1.f, 0.5f},
            borderWidth,
            rectRotation,
            rectScale,
            rectPivot,
            insetBorder); */
    }
#endif

#if 0
    {
        ZoneScopedN("Sprites");
        std::array<Sprite, 3> sprites{interactipTipSprite, talkTipSprite, kaeruSprite};

        std::default_random_engine e1(1337);
        std::uniform_int_distribution<int> uniform_dist(0, 1280);
        std::uniform_real_distribution<float> angleDist(0.f, glm::pi<float>() / 2.f);
        for (int x = 0; x < 100; ++x) {
            for (int y = 0; y < 100; ++y) {
                const auto& sprite = sprites[(x * y) % 3];
                const auto pos = glm::vec2{uniform_dist(e1), uniform_dist(e1)};
                const auto rot = angleDist(e1);
                uiRenderer.drawSprite(sprite, pos, rot);
            }
        }
    }
#endif
}

void GameUI::updateDevTools(float dt)
{
    /*
    auto dragVec2 = [](const char* label, glm::vec2& v) {
        std::array<float, 2> arr{v.x, v.y};
        if (ImGui::DragFloat2(label, arr.data())) {
            v.x = arr[0];
            v.y = arr[1];
        }
    };
    dragVec2("rectPos", rectPos);
    dragVec2("rectSize", rectSize);
    dragVec2("rectPivot", rectPivot);
    dragVec2("rectScale", rectScale);
    ImGui::DragFloat("rectRotation", &rectRotation, 0.01f, -2.f, 2.f);
    ImGui::DragFloat("borderWidth", &borderWidth, 0.1f, 1.f, 16.f);
    ImGui::Checkbox("insetBorder", &insetBorder);
    */
}
