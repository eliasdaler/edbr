#pragma once

#include <cstdint>

#include <vulkan/vulkan.h>

namespace graphics
{
void generateMipmaps(
    VkCommandBuffer cmd,
    VkImage image,
    VkExtent2D imageSize,
    std::uint32_t mipLevels);
}
