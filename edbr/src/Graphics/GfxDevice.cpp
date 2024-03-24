#include <edbr/Graphics/GfxDevice.h>

#include <array>
#include <iostream>
#include <limits>

#include <vulkan/vulkan.h>

#include <volk.h>

#include <vk_mem_alloc.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>

#include <edbr/Graphics/Vulkan/GPUBuffer.h>
#include <edbr/Graphics/Vulkan/GPUImage.h>
#include <edbr/Graphics/Vulkan/Init.h>
#include <edbr/Graphics/Vulkan/Pipelines.h>
#include <edbr/Graphics/Vulkan/Util.h>

#include <edbr/Graphics/ImageLoader.h>
#include <edbr/Graphics/MipMapGeneration.h>

namespace
{
static constexpr auto NO_TIMEOUT = std::numeric_limits<std::uint64_t>::max();
}

GfxDevice::GfxDevice() : imageCache(*this)
{}

void GfxDevice::init(SDL_Window* window, const char* appName, bool vSync)
{
    initVulkan(window, appName);
    executor = createImmediateExecutor();

    swapchain.initSyncStructures(device);

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    swapchainFormat = VK_FORMAT_B8G8R8A8_SRGB;
    swapchain.create(device, swapchainFormat, (std::uint32_t)w, (std::uint32_t)h, vSync);

    createCommandBuffers();
    imageCache.bindlessSetManager.init(device);

    { // create white texture
        std::uint32_t pixel = 0xFFFFFFFF;
        whiteTextureID = createImage(
            {
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .extent = VkExtent3D{1, 1, 1},
            },
            "white texture",
            &pixel);
    }

    // Dear ImGui
    ImGui::CreateContext();
    imGuiBackend.init(*this, swapchainFormat);
    ImGui_ImplSDL2_InitForVulkan(window);

    for (std::size_t i = 0; i < graphics::FRAME_OVERLAP; ++i) {
        frames[i].tracyVkCtx =
            TracyVkContext(physicalDevice, device, graphicsQueue, frames[i].mainCommandBuffer);
    }
}

void GfxDevice::initVulkan(SDL_Window* window, const char* appName)
{
    VK_CHECK(volkInitialize());

    instance = vkb::InstanceBuilder{}
                   .set_app_name(appName)
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
        .descriptorIndexing = true,
        .descriptorBindingSampledImageUpdateAfterBind = true,
        .descriptorBindingStorageImageUpdateAfterBind = true,
        .descriptorBindingPartiallyBound = true,
        .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
        .scalarBlockLayout = true,
        .bufferDeviceAddress = true,
    };
    const auto features13 = VkPhysicalDeviceVulkan13Features{
        .synchronization2 = true,
        .dynamicRendering = true,
    };

    physicalDevice = vkb::PhysicalDeviceSelector{instance}
                         .set_minimum_version(1, 3)
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

void GfxDevice::endFrame(VkCommandBuffer cmd, const GPUImage& drawImage)
{
    const auto& frame = getCurrentFrame();

    // we'll copy from draw image to swapchain
    vkutil::transitionImage(
        cmd,
        drawImage.image,
        VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

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
            imGuiBackend.draw(
                cmd, *this, swapchain.getImageView(swapchainImageIndex), swapchain.getExtent());
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
    imageCache.destroyImages();
    imageCache.bindlessSetManager.cleanup(device);

    for (auto& frame : frames) {
        vkDestroyCommandPool(device, frame.commandPool, 0);
        TracyVkDestroy(frame.tracyVkCtx);
    }

    // cleanup Dear ImGui
    imGuiBackend.cleanup(*this);
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    swapchain.cleanup(device);

    executor.cleanup(device);

    vkb::destroy_surface(instance, surface);
    vmaDestroyAllocator(allocator);
    vkb::destroy_device(device);
    vkb::destroy_instance(instance);
}

GPUBuffer GfxDevice::createBuffer(
    std::size_t allocSize,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memoryUsage) const
{
    const auto bufferInfo = VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = allocSize,
        .usage = usage,
    };

    const auto allocInfo = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                 // TODO: allow to set VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT when needed
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = memoryUsage,
    };

    GPUBuffer buffer{};
    VK_CHECK(vmaCreateBuffer(
        allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.info));
    if ((usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0) {
        const auto deviceAdressInfo = VkBufferDeviceAddressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = buffer.buffer,
        };
        buffer.address = vkGetBufferDeviceAddress(device, &deviceAdressInfo);
    }

    return buffer;
}

VkDeviceAddress GfxDevice::getBufferAddress(const GPUBuffer& buffer) const
{
    const auto deviceAdressInfo = VkBufferDeviceAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer.buffer,
    };
    return vkGetBufferDeviceAddress(device, &deviceAdressInfo);
}

void GfxDevice::destroyBuffer(const GPUBuffer& buffer) const
{
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
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

bool GfxDevice::deviceSupportsSamplingCount(VkSampleCountFlagBits sample) const
{
    return (supportedSampleCounts & sample) != 0;
}

VkSampleCountFlagBits GfxDevice::getMaxSupportedSamplingCount() const
{
    return highestSupportedSamples;
}

VulkanImmediateExecutor GfxDevice::createImmediateExecutor() const
{
    VulkanImmediateExecutor executor;
    executor.init(device, graphicsQueueFamily, graphicsQueue);
    return executor;
}

void GfxDevice::immediateSubmit(std::function<void(VkCommandBuffer)>&& f) const
{
    executor.immediateSubmit(std::move(f));
}

void GfxDevice::waitIdle() const
{
    VK_CHECK(vkDeviceWaitIdle(device));
}

BindlessSetManager& GfxDevice::getBindlessSetManager()
{
    return imageCache.bindlessSetManager;
}

VkDescriptorSetLayout GfxDevice::getBindlessDescSetLayout() const
{
    return imageCache.bindlessSetManager.getDescSetLayout();
}

const VkDescriptorSet& GfxDevice::getBindlessDescSet() const
{
    return imageCache.bindlessSetManager.getDescSet();
}

void GfxDevice::bindBindlessDescSet(VkCommandBuffer cmd, VkPipelineLayout layout) const
{
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout,
        0,
        1,
        &imageCache.bindlessSetManager.getDescSet(),
        0,
        nullptr);
}

ImageId GfxDevice::createImage(
    const vkutil::CreateImageInfo& createInfo,
    const char* debugName,
    void* pixelData,
    ImageId imageId)
{
    auto image = createImageRaw(createInfo);
    if (debugName) {
        vkutil::addDebugLabel(device, image.image, debugName);
    }
    if (pixelData) {
        uploadImageData(image, pixelData);
    }
    if (imageId != NULL_IMAGE_ID) {
        return imageCache.addImage(imageId, std::move(image));
    }
    return addImageToCache(std::move(image));
}

ImageId GfxDevice::loadImageFromFile(
    const std::filesystem::path& path,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap)
{
    return imageCache.loadImageFromFile(path, format, usage, mipMap);
}

const GPUImage& GfxDevice::getImage(ImageId id) const
{
    return imageCache.getImage(id);
}

ImageId GfxDevice::addImageToCache(GPUImage img)
{
    return imageCache.addImage(std::move(img));
}

GPUImage GfxDevice::createImageRaw(const vkutil::CreateImageInfo& createInfo) const
{
    std::uint32_t mipLevels = 1;
    if (createInfo.mipMap) {
        mipLevels = static_cast<std::uint32_t>(std::floor(
                        std::log2(std::max(createInfo.extent.width, createInfo.extent.height)))) +
                    1;
    }

    if (createInfo.isCubemap) {
        assert(createInfo.numLayers == 6);
        assert(!createInfo.mipMap);
        assert((createInfo.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) != 0);
    }

    const auto imgInfo = VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = createInfo.flags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = createInfo.format,
        .extent = createInfo.extent,
        .mipLevels = mipLevels,
        .arrayLayers = createInfo.numLayers,
        .samples = createInfo.samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = createInfo.usage,
    };
    const auto allocInfo = VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    GPUImage image{};
    image.format = createInfo.format;
    image.usage = createInfo.usage;
    image.extent = createInfo.extent;
    image.mipLevels = mipLevels;
    image.numLayers = createInfo.numLayers;
    image.isCubemap = createInfo.isCubemap;

    VK_CHECK(
        vmaCreateImage(allocator, &imgInfo, &allocInfo, &image.image, &image.allocation, nullptr));

    // create view
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (createInfo.format == VK_FORMAT_D32_SFLOAT) { // TODO: support other depth formats
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    auto viewType = createInfo.numLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    if (createInfo.isCubemap) {
        viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    }

    const auto viewCreateInfo = VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.image,
        .viewType = viewType,
        .format = createInfo.format,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = aspectFlag,
                .baseMipLevel = 0,
                .levelCount = mipLevels,
                .baseArrayLayer = 0,
                .layerCount = createInfo.numLayers,
            },
    };

    VK_CHECK(vkCreateImageView(device, &viewCreateInfo, nullptr, &image.imageView));

    return image;
}

void GfxDevice::uploadImageData(const GPUImage& image, void* pixelData, std::uint32_t layer) const
{
    int numChannels = 4;
    if (image.format == VK_FORMAT_R8_UNORM) {
        // FIXME: support more types
        numChannels = 1;
    }
    const auto dataSize =
        image.extent.depth * image.extent.width * image.extent.height * numChannels;

    const auto uploadBuffer = createBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    memcpy(uploadBuffer.info.pMappedData, pixelData, dataSize);

    executor.immediateSubmit([&](VkCommandBuffer cmd) {
        assert(
            (image.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) != 0 &&
            "Image needs to have VK_IMAGE_USAGE_TRANSFER_DST_BIT to upload data to it");
        vkutil::transitionImage(
            cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        const auto copyRegion = VkBufferImageCopy{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = layer,
                    .layerCount = 1,
                },
            .imageExtent = image.extent,
        };

        vkCmdCopyBufferToImage(
            cmd,
            uploadBuffer.buffer,
            image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion);

        if (image.mipLevels > 1) {
            assert(
                (image.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) != 0 &&
                (image.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0 &&
                "Image needs to have VK_IMAGE_USAGE_TRANSFER_{DST,SRC}_BIT to generate mip maps");
            graphics::generateMipmaps(
                cmd,
                image.image,
                VkExtent2D{image.extent.width, image.extent.height},
                image.mipLevels);
        } else {
            vkutil::transitionImage(
                cmd,
                image.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    });

    destroyBuffer(uploadBuffer);
}

GPUImage GfxDevice::loadImageFromFileRaw(
    const std::filesystem::path& path,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap) const
{
    auto data = util::loadImage(path);
    assert(data.pixels);

    const auto image = createImageRaw({
        .format = format,
        .usage = usage | //
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | // for uploading pixel data to image
                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT, // for generating mips
        .extent =
            VkExtent3D{
                .width = (std::uint32_t)data.width,
                .height = (std::uint32_t)data.height,
                .depth = 1,
            },
        .mipMap = mipMap,
    });
    uploadImageData(image, data.pixels);

    vkutil::addDebugLabel(device, image.image, path.string().c_str());

    return image;
}

void GfxDevice::destroyImage(const GPUImage& image) const
{
    vkDestroyImageView(device, image.imageView, nullptr);
    vmaDestroyImage(allocator, image.image, image.allocation);
    // TODO: if image has bindless id, update the set
}
