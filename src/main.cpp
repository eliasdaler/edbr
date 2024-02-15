#include <cassert>

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include <VkBootstrap.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#define VK_CHECK(call)                 \
    do {                               \
        VkResult result_ = call;       \
        assert(result_ == VK_SUCCESS); \
    } while (0)

std::vector<VkImage> getSwapchainImages(VkDevice device, VkSwapchainKHR swapchain)
{
    std::uint32_t swapchainImageCount{};
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr));

    std::vector<VkImage> swapchainImages(swapchainImageCount);
    VK_CHECK(
        vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data()));
    return swapchainImages;
}

struct FrameData {
    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;

    VkSemaphore swapchainSemaphore;
    VkSemaphore renderSemaphore;
    VkFence renderFence;
};

VkFenceCreateInfo createFence(VkFenceCreateFlags flags = 0)
{
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;

    info.flags = flags;

    return info;
}

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

constexpr std::size_t FRAME_OVERLAP = 2;

class Renderer {
public:
    std::array<FrameData, FRAME_OVERLAP> frames{};
    std::uint32_t frameNumber{0};
    VkQueue graphicsQueue;
    std::uint32_t graphicsQueueFamily;

    FrameData& getCurrentFrame() { return frames[frameNumber % FRAME_OVERLAP]; }

    void createCommandBuffers(VkDevice device)
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

    void initSyncStructures(VkDevice device)
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
            VK_CHECK(
                vkCreateSemaphore(device, &semaphoreCreateInfo, 0, &frames[i].swapchainSemaphore));
            VK_CHECK(
                vkCreateSemaphore(device, &semaphoreCreateInfo, 0, &frames[i].renderSemaphore));
        }
    }

    void destroyCommandBuffers(VkDevice device)
    {
        vkDeviceWaitIdle(device);

        for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
            vkDestroyCommandPool(device, frames[i].commandPool, 0);
        }
    }

    void destroySyncStructures(VkDevice device)
    {
        for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
            vkDestroyFence(device, frames[i].renderFence, 0);
            vkDestroySemaphore(device, frames[i].swapchainSemaphore, 0);
            vkDestroySemaphore(device, frames[i].renderSemaphore, 0);
        }
    }

    void draw(
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
        transitionImage(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        { // clear
            auto flash = std::abs(std::sin(frameNumber / 120.f));
            auto clearValue = VkClearColorValue{{0.0f, 0.0f, flash, 1.0f}};
            auto clearRange = makeSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
            vkCmdClearColorImage(cmd, image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
        }

        transitionImage(cmd, image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        VK_CHECK(vkEndCommandBuffer(cmd));

        { // submit
            auto cmdinfo = commandBufferSubmitInfo(cmd);
            auto waitInfo = semaphoreSubmitInfo(
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                currentFrame.swapchainSemaphore);
            auto signalInfo = semaphoreSubmitInfo(
                VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, currentFrame.renderSemaphore);

            auto submit = submitInfo(&cmdinfo, &signalInfo, &waitInfo);
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

        // increase the number of frames drawn
        frameNumber++;
    }
};

int main()
{
    static constexpr std::uint32_t SCREEN_WIDTH = 1024;
    static constexpr std::uint32_t SCREEN_HEIGHT = 768;

    vkb::InstanceBuilder builder;
    auto instance = builder.set_app_name("Vulkan app")
                        .request_validation_layers()
                        .use_default_debug_messenger()
                        .require_api_version(1, 3, 0)
                        .build()
                        .value();

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Vulkan app", nullptr, nullptr);
    assert(window);

    vkb::PhysicalDeviceSelector selector{instance};

    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, window, 0, &surface));

    auto physicalDevice = selector.set_minimum_version(1, 3).set_surface(surface).select().value();

    vkb::DeviceBuilder device_builder{physicalDevice};
    VkPhysicalDeviceSynchronization2Features enabledFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .synchronization2 = true,
    };
    auto vkb_device = device_builder.add_pNext(&enabledFeatures).build().value();
    auto device = vkb_device.device;

    auto graphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    auto graphicsQueueFamily = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    vkb::SwapchainBuilder swapchainBuilder{vkb_device};
    auto swapchain = swapchainBuilder.use_default_format_selection()
                         .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                         .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                         .set_desired_extent(SCREEN_WIDTH, SCREEN_HEIGHT)
                         .build()
                         .value();

    auto swapchainImages = getSwapchainImages(device, swapchain);

    Renderer renderer{
        .graphicsQueue = graphicsQueue,
        .graphicsQueueFamily = graphicsQueueFamily,
    };
    renderer.createCommandBuffers(device);
    renderer.initSyncStructures(device);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer.draw(device, swapchain, swapchainImages);
    }

    renderer.destroyCommandBuffers(device);
    renderer.destroySyncStructures(device);

    vkb::destroy_swapchain(swapchain);
    vkb::destroy_surface(instance, surface);
    vkb::destroy_device(vkb_device);
    vkb::destroy_instance(instance);

    glfwDestroyWindow(window);
    glfwTerminate();
}
