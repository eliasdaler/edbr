#include <edbr/Graphics/Bouncer.h>

#include <fmt/printf.h>

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
