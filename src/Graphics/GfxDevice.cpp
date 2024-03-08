#include "GfxDevice.h"

#include <array>
#include <iostream>
#include <limits>

#include <vulkan/vulkan.h>

#include <volk.h>

#include <vk_mem_alloc.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <Graphics/Vulkan/Init.h>
#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>

namespace
{
static constexpr auto NO_TIMEOUT = std::numeric_limits<std::uint64_t>::max();
}

void GfxDevice::init(SDL_Window* window, bool vSync)
{
    initVulkan(window);
    executor = createImmediateExecutor();

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    createSwapchain((std::uint32_t)w, (std::uint32_t)h, vSync);

    createCommandBuffers();
    initSyncStructures();
    initDescriptorAllocator();

    { // Dear ImGui
        vkutil::initSwapchainViews(imguiData, device, swapchainImages);
        vkutil::initImGui(
            imguiData,
            instance,
            physicalDevice,
            device,
            graphicsQueueFamily,
            graphicsQueue,
            window);
    }

    for (std::size_t i = 0; i < graphics::FRAME_OVERLAP; ++i) {
        frames[i].tracyVkCtx =
            TracyVkContext(physicalDevice, device, graphicsQueue, frames[i].mainCommandBuffer);
        deletionQueue.pushFunction([this, i](VkDevice) { TracyVkDestroy(frames[i].tracyVkCtx); });
    }
}

void GfxDevice::initVulkan(SDL_Window* window)
{
    VK_CHECK(volkInitialize());

    instance = vkb::InstanceBuilder{}
                   .set_app_name("Vulkan app")
                   .request_validation_layers()
                   .use_default_debug_messenger()
                   .require_api_version(1, 3, 0)
                   .build()
                   .value();

    volkLoadInstance(instance);

    auto res = SDL_Vulkan_CreateSurface(window, instance, &surface);
    if (res != SDL_TRUE) {
        std::cout << "Failed to create Vulkan surface: " << SDL_GetError() << std::endl;
        std::exit(1);
    }

    const auto deviceFeatures = VkPhysicalDeviceFeatures{
        .depthClamp = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
    };

    const auto features12 = VkPhysicalDeviceVulkan12Features{
        .bufferDeviceAddress = true,
    };
    const auto features13 = VkPhysicalDeviceVulkan13Features{
        .synchronization2 = true,
        .dynamicRendering = true,
    };

    physicalDevice = vkb::PhysicalDeviceSelector{instance}
                         .set_minimum_version(1, 3)
                         .add_required_extension(VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME)
                         .set_required_features(deviceFeatures)
                         .set_required_features_12(features12)
                         .set_required_features_13(features13)
                         .set_surface(surface)
                         .select()
                         .value();

    checkDeviceCapabilities();

    device = vkb::DeviceBuilder{physicalDevice}.build().value();

    graphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();
    graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();

    { // Init VMA
        const auto vulkanFunctions = VmaVulkanFunctions{
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        };

        const auto allocatorInfo = VmaAllocatorCreateInfo{
            .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
            .physicalDevice = physicalDevice,
            .device = device,
            .pVulkanFunctions = &vulkanFunctions,
            .instance = instance,
        };
        vmaCreateAllocator(&allocatorInfo, &allocator);
    }

    deletionQueue.pushFunction([&](VkDevice) { vmaDestroyAllocator(allocator); });
}

void GfxDevice::checkDeviceCapabilities()
{
    // check limits
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);

    maxSamplerAnisotropy = props.limits.maxSamplerAnisotropy;

    { // store which sampling counts HW supports
        const auto counts = std::array{
            VK_SAMPLE_COUNT_1_BIT,
            VK_SAMPLE_COUNT_2_BIT,
            VK_SAMPLE_COUNT_4_BIT,
            VK_SAMPLE_COUNT_8_BIT,
            VK_SAMPLE_COUNT_16_BIT,
            VK_SAMPLE_COUNT_32_BIT,
            VK_SAMPLE_COUNT_64_BIT,
        };

        const auto supportedByDepthAndColor =
            props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
        supportedSampleCounts = {};
        for (const auto& count : counts) {
            if (supportedByDepthAndColor & count) {
                supportedSampleCounts = (VkSampleCountFlagBits)(supportedSampleCounts | count);
                highestSupportedSamples = count;
            }
        }
    }
}

void GfxDevice::createSwapchain(std::uint32_t width, std::uint32_t height, bool vSync)
{
    // vSync = false;
    const std::array<VkFormat, 2> formats{{
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_B8G8R8A8_UNORM,
    }};
    auto formatList = VkImageFormatListCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO,
        .viewFormatCount = 2,
        .pViewFormats = formats.data(),
    };
    swapchain = vkb::SwapchainBuilder{device}
                    .add_pNext(&formatList)
                    .set_desired_format(VkSurfaceFormatKHR{
                        .format = VK_FORMAT_B8G8R8A8_SRGB,
                        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
                    })
                    .set_create_flags(VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR)
                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                    .set_desired_present_mode(
                        vSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
                    .set_desired_extent(width, height)
                    .build()
                    .value();
    swapchainExtent = VkExtent2D{.width = width, .height = height};

    swapchainImages = swapchain.get_images().value();
    swapchainImageViews = swapchain.get_image_views().value();

    // TODO: if re-creation of swapchain is supported, don't forget to call
    // vkutil::initSwapchainViews here.
}

void GfxDevice::createCommandBuffers()
{
    const auto poolCreateInfo = vkinit::
        commandPoolCreateInfo(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphicsQueueFamily);

    for (std::uint32_t i = 0; i < graphics::FRAME_OVERLAP; ++i) {
        auto& commandPool = frames[i].commandPool;
        VK_CHECK(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool));

        const auto cmdAllocInfo = vkinit::commandBufferAllocateInfo(commandPool, 1);
        auto& mainCommandBuffer = frames[i].mainCommandBuffer;
        VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &mainCommandBuffer));
    }
}

void GfxDevice::initSyncStructures()
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

void GfxDevice::initDescriptorAllocator()
{
    const auto sizes = std::vector<DescriptorAllocatorGrowable::PoolSizeRatio>{
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
    };

    descriptorAllocator.init(device, 10, sizes);
}

VkCommandBuffer GfxDevice::beginFrame()
{
    const auto& frame = getCurrentFrame();

    VK_CHECK(vkWaitForFences(device, 1, &frame.renderFence, true, NO_TIMEOUT));
    VK_CHECK(vkResetFences(device, 1, &frame.renderFence));

    const auto& cmd = frame.mainCommandBuffer;
    const auto cmdBeginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    return cmd;
}

void GfxDevice::endFrame(VkCommandBuffer cmd, const AllocatedImage& drawImage)
{
    const auto& frame = getCurrentFrame();

    // get swapchain image
    std::uint32_t swapchainImageIndex{};
    VK_CHECK(vkAcquireNextImageKHR(
        device,
        swapchain,
        NO_TIMEOUT,
        frame.swapchainSemaphore,
        VK_NULL_HANDLE,
        &swapchainImageIndex));
    const auto& swapchainImage = swapchainImages[swapchainImageIndex];

    // copy from draw image into swapchain
    vkutil::transitionImage(
        cmd, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkutil::copyImageToImage(
        cmd,
        drawImage.image,
        swapchainImage,
        {drawImage.extent.width, drawImage.extent.height},
        swapchainExtent);

    if (imguiDrawn) {
        vkutil::transitionImage(
            cmd,
            swapchainImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        { // draw Dear ImGui
            TracyVkZoneC(frame.tracyVkCtx, cmd, "ImGui", tracy::Color::VioletRed);
            vkutil::cmdBeginLabel(cmd, "Draw Dear ImGui");
            vkutil::drawImGui(imguiData, cmd, swapchainExtent, swapchainImageIndex);
            vkutil::cmdEndLabel(cmd);
        }

        // prepare for present
        vkutil::transitionImage(
            cmd,
            swapchainImage,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    } else {
        // prepare for present
        vkutil::transitionImage(
            cmd,
            swapchainImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    // TODO: don't collect every frame?
    TracyVkCollect(frame.tracyVkCtx, frame.mainCommandBuffer);

    VK_CHECK(vkEndCommandBuffer(cmd));

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
    frameNumber++;
}

void GfxDevice::cleanup()
{
    for (auto& frame : frames) {
        vkDestroyCommandPool(device, frame.commandPool, 0);
        vkDestroyFence(device, frame.renderFence, nullptr);
        vkDestroySemaphore(device, frame.swapchainSemaphore, nullptr);
        vkDestroySemaphore(device, frame.renderSemaphore, nullptr);
    }

    deletionQueue.flush(device);

    vkutil::cleanupImGui(imguiData, device);

    descriptorAllocator.destroyPools(device);

    { // destroy swapchain and its views
        for (auto imageView : swapchainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        swapchainImageViews.clear();

        vkb::destroy_swapchain(swapchain);
    }

    executor.cleanup(device);

    vkb::destroy_surface(instance, surface);
    vkb::destroy_device(device);
    vkb::destroy_instance(instance);
}

AllocatedBuffer GfxDevice::createBuffer(
    std::size_t allocSize,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memoryUsage) const
{
    return vkutil::createBuffer(allocator, allocSize, usage, memoryUsage);
}

VkDeviceAddress GfxDevice::getBufferAddress(const AllocatedBuffer& buffer) const
{
    const auto deviceAdressInfo = VkBufferDeviceAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer.buffer,
    };
    return vkGetBufferDeviceAddress(device, &deviceAdressInfo);
}

void GfxDevice::destroyBuffer(const AllocatedBuffer& buffer) const
{
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

AllocatedImage GfxDevice::createImage(const vkutil::CreateImageInfo& createInfo) const
{
    return vkutil::createImage(device, allocator, createInfo);
}

void GfxDevice::uploadImageData(const AllocatedImage& image, void* pixelData, std::uint32_t layer)
    const
{
    vkutil::uploadImageData(
        device, graphicsQueueFamily, graphicsQueue, allocator, image, pixelData, layer);
}

AllocatedImage GfxDevice::loadImageFromFile(
    const std::filesystem::path& path,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap) const
{
    return vkutil::loadImageFromFile(
        device, graphicsQueueFamily, graphicsQueue, allocator, path, format, usage, mipMap);
}

void GfxDevice::destroyImage(const AllocatedImage& image) const
{
    vkutil::destroyImage(device, allocator, image);
}

GfxDevice::FrameData& GfxDevice::getCurrentFrame()
{
    return frames[getCurrentFrameIndex()];
}

std::uint32_t GfxDevice::getCurrentFrameIndex() const
{
    return frameNumber % graphics::FRAME_OVERLAP;
}

VkDescriptorSet GfxDevice::allocateDescriptorSet(VkDescriptorSetLayout layout)
{
    return descriptorAllocator.allocate(device, layout);
}

bool GfxDevice::deviceSupportsSamplingCount(VkSampleCountFlagBits sample) const
{
    return (supportedSampleCounts & sample) != 0;
}

VkSampleCountFlagBits GfxDevice::getHighestSupportedSamplingCount() const
{
    return highestSupportedSamples;
}

VulkanImmediateExecutor GfxDevice::createImmediateExecutor() const
{
    VulkanImmediateExecutor executor;
    executor.init(device, graphicsQueueFamily, graphicsQueue);
    return executor;
}
