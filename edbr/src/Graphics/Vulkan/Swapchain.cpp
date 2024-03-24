#include <edbr/Graphics/Vulkan/Swapchain.h>

#include <volk.h>

#include <edbr/Graphics/Vulkan/Init.h>
#include <edbr/Graphics/Vulkan/Util.h>

namespace
{
static constexpr auto NO_TIMEOUT = std::numeric_limits<std::uint64_t>::max();
}

void Swapchain::initSyncStructures(VkDevice device)
{
    const auto fenceCreateInfo = VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    const auto semaphoreCreateInfo = VkSemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    for (std::uint32_t i = 0; i < graphics::FRAME_OVERLAP; ++i) {
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));
        VK_CHECK(vkCreateSemaphore(
            device, &semaphoreCreateInfo, nullptr, &frames[i].swapchainSemaphore));
        VK_CHECK(
            vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));
    }
}

void Swapchain::create(
    const vkb::Device& device,
    VkFormat swapchainFormat,
    std::uint32_t width,
    std::uint32_t height,
    bool vSync)
{
    assert(swapchainFormat == VK_FORMAT_B8G8R8A8_SRGB && "TODO: test other formats");
    // vSync = false;
    swapchain = vkb::SwapchainBuilder{device}
                    .set_desired_format(VkSurfaceFormatKHR{
                        .format = swapchainFormat,
                        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
                    })
                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                    .set_desired_present_mode(
                        vSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
                    .set_desired_extent(width, height)
                    .build()
                    .value();
    extent = VkExtent2D{.width = width, .height = height};

    images = swapchain.get_images().value();
    imageViews = swapchain.get_image_views().value();

    // TODO: if re-creation of swapchain is supported, don't forget to call
    // vkutil::initSwapchainViews here.
}

void Swapchain::cleanup(VkDevice device)
{
    for (auto& frame : frames) {
        vkDestroyFence(device, frame.renderFence, nullptr);
        vkDestroySemaphore(device, frame.swapchainSemaphore, nullptr);
        vkDestroySemaphore(device, frame.renderSemaphore, nullptr);
    }

    { // destroy swapchain and its views
        for (auto imageView : imageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        imageViews.clear();

        vkb::destroy_swapchain(swapchain);
    }
}

void Swapchain::beginFrame(VkDevice device, std::size_t frameIndex) const
{
    auto& frame = frames[frameIndex];
    VK_CHECK(vkWaitForFences(device, 1, &frame.renderFence, true, NO_TIMEOUT));
    VK_CHECK(vkResetFences(device, 1, &frame.renderFence));
}

std::pair<VkImage, std::uint32_t> Swapchain::acquireImage(VkDevice device, std::size_t frameIndex)
    const
{
    std::uint32_t swapchainImageIndex{};
    VK_CHECK(vkAcquireNextImageKHR(
        device,
        swapchain,
        NO_TIMEOUT,
        frames[frameIndex].swapchainSemaphore,
        VK_NULL_HANDLE,
        &swapchainImageIndex));
    return {images[swapchainImageIndex], swapchainImageIndex};
}

void Swapchain::submitAndPresent(
    VkCommandBuffer cmd,
    VkQueue graphicsQueue,
    std::size_t frameIndex,
    std::uint32_t swapchainImageIndex) const
{
    const auto& frame = frames[frameIndex];

    { // submit
        const auto submitInfo = VkCommandBufferSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = cmd,
        };
        const auto waitInfo = vkinit::semaphoreSubmitInfo(
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame.swapchainSemaphore);
        const auto signalInfo = vkinit::
            semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame.renderSemaphore);

        const auto submit = vkinit::submitInfo(&submitInfo, &waitInfo, &signalInfo);
        VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, frame.renderFence));
    }

    { // present
        const auto presentInfo = VkPresentInfoKHR{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frame.renderSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain.swapchain,
            .pImageIndices = &swapchainImageIndex,
        };

        VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));
    }
}
