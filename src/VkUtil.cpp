#include "VkUtil.h"

#include <cstdint>

namespace vkutil
{

VkImageSubresourceRange makeSubresourceRange(VkImageAspectFlags aspectMask)
{
    return VkImageSubresourceRange{
        .aspectMask = aspectMask,
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };
}

std::vector<VkImage> getSwapchainImages(VkDevice device, VkSwapchainKHR swapchain)
{
    std::uint32_t swapchainImageCount{};
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr));

    std::vector<VkImage> swapchainImages(swapchainImageCount);
    VK_CHECK(
        vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data()));
    return swapchainImages;
}

void transitionImage(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout currentLayout,
    VkImageLayout newLayout)
{
    VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ?
                                        VK_IMAGE_ASPECT_DEPTH_BIT :
                                        VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageMemoryBarrier2 imageBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
        .oldLayout = currentLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = makeSubresourceRange(aspectMask),
    };

    VkDependencyInfo depInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier,
    };

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

VkFenceCreateInfo createFence(VkFenceCreateFlags flags)
{
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;

    info.flags = flags;

    return info;
}

VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore)
{
    return VkSemaphoreSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = semaphore,
        .value = 1,
        .stageMask = stageMask,
        .deviceIndex = 0,
    };
}

VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd)
{
    return VkCommandBufferSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cmd,
        .deviceMask = 0,
    };
}

VkSubmitInfo2 submitInfo(
    VkCommandBufferSubmitInfo* cmd,
    VkSemaphoreSubmitInfo* signalSemaphoreInfo,
    VkSemaphoreSubmitInfo* waitSemaphoreInfo)
{
    return VkSubmitInfo2{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = waitSemaphoreInfo ? 1u : 0u,
        .pWaitSemaphoreInfos = waitSemaphoreInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = cmd,
        .signalSemaphoreInfoCount = signalSemaphoreInfo ? 1u : 0u,
        .pSignalSemaphoreInfos = signalSemaphoreInfo,
    };
}

}
