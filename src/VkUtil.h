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
VkImageSubresourceRange makeSubresourceRange(VkImageAspectFlags aspectMask);
std::vector<VkImage> getSwapchainImages(VkDevice device, VkSwapchainKHR swapchain);
void transitionImage(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout currentLayout,
    VkImageLayout newLayout);

VkFenceCreateInfo createFence(VkFenceCreateFlags flags = 0);
VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd);
VkSubmitInfo2 submitInfo(
    VkCommandBufferSubmitInfo* cmd,
    VkSemaphoreSubmitInfo* signalSemaphoreInfo,
    VkSemaphoreSubmitInfo* waitSemaphoreInfo);

} // end of namespace vkutil
