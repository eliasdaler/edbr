#include <edbr/Graphics/Letterbox.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace util
{
glm::vec4 calculateLetterbox(
    const glm::ivec2& inputSize,
    const glm::ivec2& outputSize,
    bool integerScale)
{
    if (integerScale &&
        // integer scaling won't work if inputSize > outputSize
        outputSize.x >= inputSize.x && outputSize.y >= inputSize.y) {
        auto scale = std::min(outputSize.x / (float)inputSize.x, outputSize.y / (float)inputSize.y);
        scale = std::floor(scale);

        if (scale != 0.f) {
            const auto realSize = inputSize * (int)scale;
            auto tl = (outputSize - realSize) / 2;
            return {tl.x, tl.y, inputSize.x * scale, inputSize.y * scale};
        }
    }

    const auto inputAspect = inputSize.x / (float)inputSize.y;
    const auto outputAspect = outputSize.x / (float)outputSize.y;

    const auto resWidth = (outputAspect > inputAspect) ? // output proportionally wider
                                                         // than input?
                              (outputSize.y * inputAspect) : // take portion of width
                              (outputSize.x); // take up entire width

    const auto resHeight = (outputAspect < inputAspect) ? // output proportionally taller
                                                          // than input?
                               (outputSize.x / inputAspect) : // take portion of height
                               (outputSize.y); // take up entire height

    return {
        (outputSize.x - resWidth) / 2.0f, // center horizontally.
        (outputSize.y - resHeight) / 2.0f, // center vertically.
        resWidth,
        resHeight,
    };
}
}
