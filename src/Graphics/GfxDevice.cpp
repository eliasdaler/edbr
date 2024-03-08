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

    swapchain.initSyncStructures(device);

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    swapchain.create(device, (std::uint32_t)w, (std::uint32_t)h, vSync);

    createCommandBuffers();
    initDescriptorAllocator();

    { // Dear ImGui
        vkutil::initSwapchainViews(imguiData, device, swapchain.getImages());
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
    swapchain.beginFrame(device, getCurrentFrameIndex());

    const auto& frame = getCurrentFrame();
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
    const auto [swapchainImage, swapchainImageIndex] =
        swapchain.acquireImage(device, getCurrentFrameIndex());

    // copy from draw image into swapchain
    vkutil::transitionImage(
        cmd, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkutil::copyImageToImage(
        cmd, drawImage.image, swapchainImage, drawImage.getExtent2D(), swapchain.getExtent());

    if (imguiDrawn) {
        vkutil::transitionImage(
            cmd,
            swapchainImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        { // draw Dear ImGui
            TracyVkZoneC(frame.tracyVkCtx, cmd, "ImGui", tracy::Color::VioletRed);
            vkutil::cmdBeginLabel(cmd, "Draw Dear ImGui");
            vkutil::drawImGui(imguiData, cmd, swapchain.getExtent(), swapchainImageIndex);
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

    swapchain.submitAndPresent(cmd, graphicsQueue, getCurrentFrameIndex(), swapchainImageIndex);

    frameNumber++;
}

void GfxDevice::cleanup()
{
    for (auto& frame : frames) {
        vkDestroyCommandPool(device, frame.commandPool, 0);
    }

    deletionQueue.flush(device);

    vkutil::cleanupImGui(imguiData, device);
    swapchain.cleanup(device);

    descriptorAllocator.destroyPools(device);

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

const TracyVkCtx& GfxDevice::getTracyVkCtx() const
{
    return frames[getCurrentFrameIndex()].tracyVkCtx;
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
