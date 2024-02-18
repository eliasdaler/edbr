#include "Renderer.h"

#include <VkInit.h>
#include <VkPipelines.h>
#include <VkUtil.h>

#include <array>
#include <cmath>
#include <iostream>
#include <limits>

#include <vulkan/vulkan.h>

#include <VkBootstrap.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace
{
static constexpr std::uint32_t SCREEN_WIDTH = 1920;
static constexpr std::uint32_t SCREEN_HEIGHT = 1080;
}

void Renderer::init()
{
    initVulkan();
    createSwapchain(SCREEN_WIDTH, SCREEN_HEIGHT);
    createCommandBuffers();
    initSyncStructures();
    initDescriptors();
    initPipelines();
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

    VkPhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = true;

    VkPhysicalDeviceVulkan13Features features13{};
    features13.synchronization2 = true;

    vkb::PhysicalDeviceSelector selector{instance};
    auto physicalDevice = selector.set_minimum_version(1, 3)
                              .set_required_features_12(features12)
                              .set_required_features_13(features13)
                              .set_surface(surface)
                              .select()
                              .value();

    vkb::DeviceBuilder device_builder{physicalDevice};
    device = device_builder.build().value();

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
    swapchainExtent = VkExtent2D{.width = width, .height = height};

    swapchainImages = vkutil::getSwapchainImages(device, swapchain);

    const auto extent = VkExtent3D{
        .width = width,
        .height = height,
        .depth = 1,
    };
    drawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    drawImage.extent = extent;

    VkImageUsageFlags usages{};
    usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    usages |= VK_IMAGE_USAGE_STORAGE_BIT;
    usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo imgCreateInfo = vkinit::imageCreateInfo(drawImage.format, usages, extent);

    VmaAllocationCreateInfo imgAllocInfo{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    vmaCreateImage(
        allocator, &imgCreateInfo, &imgAllocInfo, &drawImage.image, &drawImage.allocation, nullptr);

    const auto imageViewCreateInfo =
        vkinit::imageViewCreateInfo(drawImage.format, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, 0, &drawImage.imageView));

    deletionQueue.pushFunction([this]() {
        vkDestroyImageView(device, drawImage.imageView, nullptr);
        vmaDestroyImage(allocator, drawImage.image, drawImage.allocation);
    });
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

void Renderer::initDescriptors()
{
    const auto sizes = std::array{
        DescriptorAllocator::PoolSizeRatio{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .ratio = 1.f,
        },
    };

    descriptorAllocator.initPool(device, 10, sizes);

    DescriptorLayoutBuilder builder{};
    builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    drawImageDescriptorLayout = builder.build(device, VK_SHADER_STAGE_COMPUTE_BIT);

    deletionQueue.pushFunction(
        [this]() { vkDestroyDescriptorSetLayout(device, drawImageDescriptorLayout, nullptr); });

    drawImageDescriptors = descriptorAllocator.allocate(device, drawImageDescriptorLayout);

    const auto imgInfo = VkDescriptorImageInfo{
        .imageView = drawImage.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    const auto drawImageWrite = VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = drawImageDescriptors,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &imgInfo,
    };

    vkUpdateDescriptorSets(device, 1, &drawImageWrite, 0, nullptr);
}

void Renderer::initPipelines()
{
    initBackgroundPipelines();
}

void Renderer::initBackgroundPipelines()
{
    auto layout = VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &drawImageDescriptorLayout,
    };

    VK_CHECK(vkCreatePipelineLayout(device, &layout, 0, &gradientPipelineLayout));

    VkShaderModule shader;
    if (!vkutil::loadShaderModule("shaders/gradient.comp.spv", device, &shader)) {
        std::cout << "Error while building compute shader" << std::endl;
        assert(false);
    }

    auto pipelineCreateInfo = VkComputePipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = shader,
                .pName = "main",
            },
        .layout = gradientPipelineLayout,
    };

    VK_CHECK(vkCreateComputePipelines(
        device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, 0, &gradientPipeline));

    vkDestroyShaderModule(device, shader, nullptr);
    deletionQueue.pushFunction([this]() {
        vkDestroyPipelineLayout(device, gradientPipelineLayout, nullptr);
        vkDestroyPipeline(device, gradientPipeline, nullptr);
    });
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

    drawExtent.width = drawImage.extent.width;
    drawExtent.height = drawImage.extent.height;

    VkCommandBuffer cmd = currentFrame.mainCommandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    vkutil::
        transitionImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    drawBackground(cmd);

    vkutil::transitionImage(
        cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // get swapchain image
    std::uint32_t swapchainImageIndex{};
    VK_CHECK(vkAcquireNextImageKHR(
        device,
        swapchain,
        defaultTimeout,
        currentFrame.swapchainSemaphore,
        0,
        &swapchainImageIndex));
    auto& swapchainImage = swapchainImages[swapchainImageIndex];

    // copy from draw image
    vkutil::transitionImage(
        cmd, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkutil::copyImageToImage(cmd, drawImage.image, swapchainImage, drawExtent, swapchainExtent);
    // prepare for present
    vkutil::transitionImage(
        cmd, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));

    { // submit
        auto cmdinfo = vkinit::commandBufferSubmitInfo(cmd);
        auto waitInfo = vkinit::semaphoreSubmitInfo(
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, currentFrame.swapchainSemaphore);
        auto signalInfo = vkinit::
            semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, currentFrame.renderSemaphore);

        auto submit = vkinit::submitInfo(&cmdinfo, &signalInfo, &waitInfo);
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

void Renderer::drawBackground(VkCommandBuffer cmd)
{
    /* auto flash = std::abs(std::sin(frameNumber / 120.f));
    auto clearValue = VkClearColorValue{{0.0f, 0.0f, flash, 1.0f}};
    auto clearRange = vkinit::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(
        cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange); */

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipeline);
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        gradientPipelineLayout,
        0,
        1,
        &drawImageDescriptors,
        0,
        nullptr);

    vkCmdDispatch(
        cmd, std::ceil(drawExtent.width / 16.f), std::ceil(drawExtent.height / 16.f), 1.f);
}

void Renderer::cleanup()
{
    vkDeviceWaitIdle(device);
    deletionQueue.flush();

    destroyCommandBuffers();
    destroySyncStructures();

    descriptorAllocator.destroyPool(device);

    vkb::destroy_swapchain(swapchain);
    vkb::destroy_surface(instance, surface);
    vkb::destroy_device(device);
    vkb::destroy_instance(instance);

    glfwDestroyWindow(window);
    glfwTerminate();
}
