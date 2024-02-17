#include "Renderer.h"

#include <VkUtil.h>

#include <cmath>
#include <limits>

Renderer::Renderer(VkQueue graphicsQueue, std::uint32_t graphicsQueueFamily) :
    graphicsQueue(graphicsQueue), graphicsQueueFamily(graphicsQueueFamily)
{}

void Renderer::createCommandBuffers(VkDevice device)
{
    VkCommandPoolCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphicsQueueFamily,
    };
    for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        auto& commandPool = frames[i].commandPool;
        VK_CHECK(vkCreateCommandPool(device, &createInfo, 0, &commandPool));
        VkCommandBufferAllocateInfo cmdAllocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        auto& mainCommandBuffer = frames[i].mainCommandBuffer;
        VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &mainCommandBuffer));
    }
}

void Renderer::initSyncStructures(VkDevice device)
{
    const auto fenceCreateInfo = VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    const auto semaphoreCreateInfo = VkSemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, 0, &frames[i].renderFence));
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, 0, &frames[i].swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, 0, &frames[i].renderSemaphore));
    }
}

void Renderer::destroyCommandBuffers(VkDevice device)
{
    vkDeviceWaitIdle(device);

    for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        vkDestroyCommandPool(device, frames[i].commandPool, 0);
    }
}

void Renderer::destroySyncStructures(VkDevice device)
{
    for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        vkDestroyFence(device, frames[i].renderFence, 0);
        vkDestroySemaphore(device, frames[i].swapchainSemaphore, 0);
        vkDestroySemaphore(device, frames[i].renderSemaphore, 0);
    }
}

Renderer::FrameData& Renderer::getCurrentFrame()
{
    return frames[frameNumber % FRAME_OVERLAP];
}

void Renderer::draw(
    VkDevice device,
    VkSwapchainKHR swapchain,
    const std::vector<VkImage>& swapchainImages)
{
    static constexpr auto defaultTimeout = std::numeric_limits<std::uint64_t>::max();

    const auto& currentFrame = getCurrentFrame();
    VK_CHECK(vkWaitForFences(device, 1, &currentFrame.renderFence, true, defaultTimeout));
    VK_CHECK(vkResetFences(device, 1, &currentFrame.renderFence));

    std::uint32_t swapchainImageIndex{};

    VK_CHECK(vkAcquireNextImageKHR(
        device,
        swapchain,
        defaultTimeout,
        currentFrame.swapchainSemaphore,
        0,
        &swapchainImageIndex));

    VkCommandBuffer cmd = currentFrame.mainCommandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    auto& image = swapchainImages[swapchainImageIndex];
    vkutil::transitionImage(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    { // clear
        auto flash = std::abs(std::sin(frameNumber / 120.f));
        auto clearValue = VkClearColorValue{{0.0f, 0.0f, flash, 1.0f}};
        auto clearRange = vkutil::makeSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdClearColorImage(cmd, image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
    }

    vkutil::transitionImage(cmd, image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    VK_CHECK(vkEndCommandBuffer(cmd));

    { // submit
        auto cmdinfo = vkutil::commandBufferSubmitInfo(cmd);
        auto waitInfo = vkutil::semaphoreSubmitInfo(
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, currentFrame.swapchainSemaphore);
        auto signalInfo = vkutil::
            semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, currentFrame.renderSemaphore);

        auto submit = vkutil::submitInfo(&cmdinfo, &signalInfo, &waitInfo);
        VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, currentFrame.renderFence));
    }

    { // present
        auto presentInfo = VkPresentInfoKHR{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &currentFrame.renderSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &swapchainImageIndex,
        };

        VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));
    }

    frameNumber++;
}
