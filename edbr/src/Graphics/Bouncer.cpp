#include <edbr/Graphics/Bouncer.h>

#include <edbr/Core/JsonDataLoader.h>

#include <fmt/printf.h>

void Bouncer::Params::load(const JsonDataLoader& loader)
{
    loader.get("maxOffset", maxOffset);
    loader.get("moveDuration", moveDuration);

    if (loader.hasKey("tweenType")) {
        std::string tweenTypeStr;
        loader.get("tweenType", tweenTypeStr);
        if (tweenTypeStr == "quadraticEaseInOut") {
            tween = glm::quadraticEaseIn<float>;
        } else {
            throw std::runtime_error(fmt::format("unknown tween type '{}'", tweenTypeStr));
        }
    }
}

Bouncer::Bouncer(Params params) :
    maxOffset(params.maxOffset), moveDuration(params.moveDuration), tween(params.tween)
{
    assert(maxOffset != 0.f);
    assert(moveDuration != 0.f);
}

void Bouncer::update(float dt)
{
    currentTime += dt;
    if (currentTime > moveDuration) {
        moveToMax = !moveToMax;
        currentTime -= moveDuration;
    }

    float offsetRel = tween(currentTime / moveDuration);
    if (moveToMax) {
        offset = -maxOffset + offsetRel * 2 * maxOffset;
    } else {
        offset = maxOffset - offsetRel * 2 * maxOffset;
    }
}
