#pragma once

#include <cassert>
#include <vector>

#include <vulkan/vulkan.h>

#define VK_CHECK(call)                 \
    do {                               \
        VkResult result_ = call;       \
        assert(result_ == VK_SUCCESS); \
    } while (0)

namespace vkutil
{
std::vector<VkImage> getSwapchainImages(VkDevice device, VkSwapchainKHR swapchain);
void transitionImage(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout currentLayout,
    VkImageLayout newLayout);

} // end of namespace vkutil
