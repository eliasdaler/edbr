#pragma once

#include <cstdint>

#include <vulkan/vulkan.h>

namespace vkinit
{
VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask);
VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

VkCommandPoolCreateInfo commandPoolCreateInfo(
    VkCommandPoolCreateFlags flags,
    std::uint32_t queueFamilyIndex);

VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd);
VkCommandBufferAllocateInfo commandBufferAllocateInfo(
    VkCommandPool commandPool,
    std::uint32_t commandBufferCount);

VkSubmitInfo2 submitInfo(
    const VkCommandBufferSubmitInfo* cmd,
    const VkSemaphoreSubmitInfo* signalSemaphoreInfo,
    const VkSemaphoreSubmitInfo* waitSemaphoreInfo);

VkImageCreateInfo imageCreateInfo(
    VkFormat format,
    VkImageUsageFlags usageFlags,
    VkExtent3D extent,
    std::uint32_t mipLevels = 1);
VkImageViewCreateInfo imageViewCreateInfo(
    VkFormat format,
    VkImage image,
    VkImageAspectFlags aspectFlags);

VkRenderingAttachmentInfo attachmentInfo(
    VkImageView view,
    const VkClearValue* clear,
    VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

VkRenderingAttachmentInfo depthAttachmentInfo(VkImageView view, VkImageLayout layout);

VkRenderingInfo renderingInfo(
    VkExtent2D renderExtent,
    const VkRenderingAttachmentInfo* colorAttachment,
    const VkRenderingAttachmentInfo* depthAttachment);

VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(
    VkShaderStageFlagBits stage,
    VkShaderModule shaderModule);
} // end of namespace vkinit
