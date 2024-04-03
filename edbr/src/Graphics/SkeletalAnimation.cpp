#include <edbr/Graphics/SkeletalAnimation.h>

const std::vector<std::string>& SkeletalAnimation::getEventsForFrame(int frame) const
{
    if (auto it = events.find(frame); it != events.end()) {
        return it->second;
    }
    static const std::vector<std::string> emptyVector{};
    return emptyVector;
}
