#include "Renderer.h"

#include <VkUtil.h>

#include <cmath>
#include <limits>

#include <vulkan/vulkan.h>

#include <VkBootstrap.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace
{
static constexpr std::uint32_t SCREEN_WIDTH = 1024;
static constexpr std::uint32_t SCREEN_HEIGHT = 768;
}

void Renderer::init()
{
    initVulkan();
    createSwapchain(SCREEN_WIDTH, SCREEN_HEIGHT);
    createCommandBuffers();
    initSyncStructures();
}

void Renderer::initVulkan()
{
    vkb::InstanceBuilder builder;
    instance = builder.set_app_name("Vulkan app")
                   .request_validation_layers()
                   .use_default_debug_messenger()
                   .require_api_version(1, 3, 0)
                   .build()
                   .value();

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Vulkan app", nullptr, nullptr);
    assert(window);

    VK_CHECK(glfwCreateWindowSurface(instance, window, 0, &surface));

    vkb::PhysicalDeviceSelector selector{instance};
    auto physicalDevice = selector.set_minimum_version(1, 3).set_surface(surface).select().value();

    vkb::DeviceBuilder device_builder{physicalDevice};
    VkPhysicalDeviceSynchronization2Features enabledFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .synchronization2 = true,
    };
    device = device_builder.add_pNext(&enabledFeatures).build().value();

    graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();

    // init vma allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &allocator);

    deletionQueue.pushFunction([&]() { vmaDestroyAllocator(allocator); });
}

void Renderer::createSwapchain(std::uint32_t width, std::uint32_t height)
{
    vkb::SwapchainBuilder swapchainBuilder{device};
    swapchain = swapchainBuilder.use_default_format_selection()
                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                    .set_desired_extent(width, height)
                    .build()
                    .value();

    swapchainImages = vkutil::getSwapchainImages(device, swapchain);
}

void Renderer::createCommandBuffers()
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

void Renderer::initSyncStructures()
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

void Renderer::destroyCommandBuffers()
{
    vkDeviceWaitIdle(device);

    for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        vkDestroyCommandPool(device, frames[i].commandPool, 0);
    }
}

void Renderer::destroySyncStructures()
{
    for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        vkDestroyFence(device, frames[i].renderFence, 0);
        vkDestroySemaphore(device, frames[i].swapchainSemaphore, 0);
        vkDestroySemaphore(device, frames[i].renderSemaphore, 0);
    }
}

void Renderer::run()
{
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        draw();
    }
}

Renderer::FrameData& Renderer::getCurrentFrame()
{
    return frames[frameNumber % FRAME_OVERLAP];
}

void Renderer::draw()
{
    static constexpr auto defaultTimeout = std::numeric_limits<std::uint64_t>::max();

    auto& currentFrame = getCurrentFrame();
    VK_CHECK(vkWaitForFences(device, 1, &currentFrame.renderFence, true, defaultTimeout));
    currentFrame.deletionQueue.flush();

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
            .pSwapchains = &swapchain.swapchain,
            .pImageIndices = &swapchainImageIndex,
        };

        VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));
    }

    frameNumber++;
}

void Renderer::cleanup()
{
    vkDeviceWaitIdle(device);
    deletionQueue.flush();

    destroyCommandBuffers();
    destroySyncStructures();

    vkb::destroy_swapchain(swapchain);
    vkb::destroy_surface(instance, surface);
    vkb::destroy_device(device);
    vkb::destroy_instance(instance);

    glfwDestroyWindow(window);
    glfwTerminate();
}
