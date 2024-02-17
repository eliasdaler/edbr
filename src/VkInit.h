#pragma once

#include <vulkan/vulkan.h>

namespace vkinit
{
VkImageSubresourceRange subresourceRange(VkImageAspectFlags aspectMask);
VkFenceCreateInfo createFence(VkFenceCreateFlags flags = 0);
VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd);
VkSubmitInfo2 submitInfo(
    VkCommandBufferSubmitInfo* cmd,
    VkSemaphoreSubmitInfo* signalSemaphoreInfo,
    VkSemaphoreSubmitInfo* waitSemaphoreInfo);

VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
VkImageViewCreateInfo imageViewCreateInfo(
    VkFormat format,
    VkImage image,
    VkImageAspectFlags aspectFlags);

} // end of namespace vkinit
