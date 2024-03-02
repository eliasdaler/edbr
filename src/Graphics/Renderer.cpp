#include "Renderer.h"

#include <array>
#include <iostream>
#include <limits>

#include <vulkan/vulkan.h>

#define VOLK_IMPLEMENTATION
#include <volk.h>

#include <vk_mem_alloc.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <Graphics/CPUMesh.h>
#include <Graphics/Vulkan/Init.h>
#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>

#include <Math/Util.h>

namespace
{
static constexpr auto NO_TIMEOUT = std::numeric_limits<std::uint64_t>::max();
}

void Renderer::init(SDL_Window* window, bool vSync)
{
    initVulkan(window);
    executor.init(device, graphicsQueueFamily, graphicsQueue);

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    createSwapchain((std::uint32_t)w, (std::uint32_t)h, vSync);

    createCommandBuffers();
    initSyncStructures();
    initDescriptorAllocators();

    initDescriptors();
    initSamplers();
    initDefaultTextures();

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

    for (std::size_t i = 0; i < FRAME_OVERLAP; ++i) {
        frames[i].tracyVkCtx =
            TracyVkContext(physicalDevice, device, graphicsQueue, frames[i].mainCommandBuffer);
        deletionQueue.pushFunction([this, i](VkDevice) { TracyVkDestroy(frames[i].tracyVkCtx); });
    }

    allocateMaterialDataBuffer();
}

void Renderer::initVulkan(SDL_Window* window)
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

    // check limits
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    assert(props.limits.maxSamplerAnisotropy == 16.f && "TODO: use smaller aniso values");

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

void Renderer::createSwapchain(std::uint32_t width, std::uint32_t height, bool vSync)
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

void Renderer::createCommandBuffers()
{
    const auto poolCreateInfo = vkinit::
        commandPoolCreateInfo(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphicsQueueFamily);

    for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        auto& commandPool = frames[i].commandPool;
        VK_CHECK(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool));

        const auto cmdAllocInfo = vkinit::commandBufferAllocateInfo(commandPool, 1);
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
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));
        VK_CHECK(vkCreateSemaphore(
            device, &semaphoreCreateInfo, nullptr, &frames[i].swapchainSemaphore));
        VK_CHECK(
            vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));
    }
}

void Renderer::initDescriptorAllocators()
{
    for (std::size_t i = 0; i < FRAME_OVERLAP; i++) {
        // create a descriptor pool
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
        };

        auto& fd = frames[i];
        fd.frameDescriptors.init(device, 1000, frame_sizes);

        deletionQueue.pushFunction(
            [&, i](VkDevice) { frames[i].frameDescriptors.destroyPools(device); });
    }

    const auto sizes = std::vector<DescriptorAllocatorGrowable::PoolSizeRatio>{
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
    };

    descriptorAllocator.init(device, 10, sizes);
}

void Renderer::initDescriptors()
{
    const auto sceneDataBindings = std::array<DescriptorLayoutBinding, 2>{{
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    }};
    sceneDataDescriptorLayout = vkutil::buildDescriptorSetLayout(
        device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sceneDataBindings);

    deletionQueue.pushFunction([this](VkDevice) {
        vkDestroyDescriptorSetLayout(device, sceneDataDescriptorLayout, nullptr);
    });

    const auto meshMaterialBindings = std::array<DescriptorLayoutBinding, 2>{{
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    }};
    meshMaterialLayout = vkutil::
        buildDescriptorSetLayout(device, VK_SHADER_STAGE_FRAGMENT_BIT, meshMaterialBindings);

    deletionQueue.pushFunction(
        [this](VkDevice) { vkDestroyDescriptorSetLayout(device, meshMaterialLayout, nullptr); });
}

void Renderer::initSamplers()
{
    { // init nearest sampler
        const auto samplerCreateInfo = VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
        };
        VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &defaultNearestSampler));
        deletionQueue.pushFunction(
            [this](VkDevice) { vkDestroySampler(device, defaultNearestSampler, nullptr); });
    }

    { // init linear sampler
        const auto samplerCreateInfo = VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            // TODO: make possible to disable anisotropy?
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = 16.f,
        };
        VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &defaultLinearSampler));
        deletionQueue.pushFunction(
            [this](VkDevice) { vkDestroySampler(device, defaultLinearSampler, nullptr); });
    }

    { // init shadow map sampler
        const auto samplerCreateInfo = VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .compareEnable = VK_TRUE,
            .compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL,
        };
        VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &defaultShadowMapSampler));
        deletionQueue.pushFunction(
            [this](VkDevice) { vkDestroySampler(device, defaultShadowMapSampler, nullptr); });
    }
}

void Renderer::initDefaultTextures()
{
    { // create white texture
        whiteTexture = createImage({
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .extent = VkExtent3D{1, 1, 1},
        });
        vkutil::addDebugLabel(device, whiteTexture.image, "white texture");

        std::uint32_t white = 0xFFFFFFFF;
        uploadImageData(whiteTexture, (void*)&white);

        deletionQueue.pushFunction([this](VkDevice) { destroyImage(whiteTexture); });
    }
}

void Renderer::draw(
    std::function<void(VkCommandBuffer)> drawFunction,
    const AllocatedImage& drawImage)
{
    auto& frame = getCurrentFrame();

    VK_CHECK(vkWaitForFences(device, 1, &frame.renderFence, true, NO_TIMEOUT));
    VK_CHECK(vkResetFences(device, 1, &frame.renderFence));

    frame.deletionQueue.flush(device);

    const auto& cmd = frame.mainCommandBuffer;
    const auto cmdBeginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    drawFunction(cmd);

    // get swapchain image
    std::uint32_t swapchainImageIndex{};
    VK_CHECK(vkAcquireNextImageKHR(
        device,
        swapchain,
        NO_TIMEOUT,
        frame.swapchainSemaphore,
        VK_NULL_HANDLE,
        &swapchainImageIndex));
    auto& swapchainImage = swapchainImages[swapchainImageIndex];

    // copy from draw image into swapchain
    vkutil::transitionImage(
        cmd, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkutil::copyImageToImage(
        cmd,
        drawImage.image,
        swapchainImage,
        {drawImage.extent.width, drawImage.extent.height},
        swapchainExtent);

    // TODO: if ImGui not drawn, we can immediately transfer to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR here
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

void Renderer::cleanup()
{
    meshCache.cleanup(*this);
    materialCache.cleanup(*this);

    for (auto& frame : frames) {
        frame.deletionQueue.flush(device);
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

AllocatedBuffer Renderer::createBuffer(
    std::size_t allocSize,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memoryUsage) const
{
    return vkutil::createBuffer(allocator, allocSize, usage, memoryUsage);
}

VkDeviceAddress Renderer::getBufferAddress(const AllocatedBuffer& buffer) const
{
    const auto deviceAdressInfo = VkBufferDeviceAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer.buffer,
    };
    return vkGetBufferDeviceAddress(device, &deviceAdressInfo);
}

void Renderer::destroyBuffer(const AllocatedBuffer& buffer) const
{
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

AllocatedImage Renderer::createImage(const vkutil::CreateImageInfo& createInfo)
{
    return vkutil::createImage(device, allocator, createInfo);
}

void Renderer::uploadImageData(const AllocatedImage& image, void* pixelData)
{
    vkutil::
        uploadImageData(device, graphicsQueueFamily, graphicsQueue, allocator, image, pixelData);
}

AllocatedImage Renderer::loadImageFromFile(
    const std::filesystem::path& path,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap)
{
    return vkutil::loadImageFromFile(
        device, graphicsQueueFamily, graphicsQueue, allocator, path, format, usage, mipMap);
}

void Renderer::destroyImage(const AllocatedImage& image) const
{
    vkutil::destroyImage(device, allocator, image);
}

void Renderer::updateDevTools(float dt)
{}

Renderer::FrameData& Renderer::getCurrentFrame()
{
    return frames[getCurrentFrameIndex()];
}

std::uint32_t Renderer::getCurrentFrameIndex() const
{
    return frameNumber % FRAME_OVERLAP;
}

VkDescriptorSet Renderer::allocateDescriptorSet(VkDescriptorSetLayout layout)
{
    return descriptorAllocator.allocate(device, layout);
}

VkDescriptorSet Renderer::uploadSceneData(
    const GPUSceneData& sceneData,
    const AllocatedImage& shadowMap)
{
    auto& frame = getCurrentFrame();

    const auto gpuSceneDataBuffer = createBuffer(
        sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    vkutil::addDebugLabel(device, gpuSceneDataBuffer.buffer, "scene data");

    frame.deletionQueue.pushFunction(
        [this, gpuSceneDataBuffer](VkDevice) { destroyBuffer(gpuSceneDataBuffer); });
    auto* sceneDataPtr = (GPUSceneData*)gpuSceneDataBuffer.info.pMappedData;
    *sceneDataPtr = sceneData;

    const auto sceneDescriptor = frame.frameDescriptors.allocate(device, sceneDataDescriptorLayout);

    DescriptorWriter writer;
    writer.writeBuffer(
        0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeImage(
        1,
        shadowMap.imageView,
        defaultShadowMapSampler,
        VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(device, sceneDescriptor);

    return sceneDescriptor;
}

MeshId Renderer::addMesh(const CPUMesh& cpuMesh, MaterialId materialId)
{
    auto gpuMesh = GPUMesh{
        .numVertices = (std::uint32_t)cpuMesh.vertices.size(),
        .numIndices = (std::uint32_t)cpuMesh.indices.size(),
        .materialId = materialId,
        .minPos = cpuMesh.minPos,
        .maxPos = cpuMesh.maxPos,
        .hasSkeleton = cpuMesh.hasSkeleton,
    };

    std::vector<glm::vec3> positions(cpuMesh.vertices.size());
    for (std::size_t i = 0; i < cpuMesh.vertices.size(); ++i) {
        positions[i] = cpuMesh.vertices[i].position;
    }
    gpuMesh.boundingSphere = util::calculateBoundingSphere(positions);

    uploadMesh(cpuMesh, gpuMesh);

    return meshCache.addMesh(std::move(gpuMesh));
}

void Renderer::uploadMesh(const CPUMesh& cpuMesh, GPUMesh& gpuMesh) const
{
    GPUMeshBuffers buffers;

    // create index buffer
    const auto indexBufferSize = cpuMesh.indices.size() * sizeof(std::uint32_t);
    buffers.indexBuffer = createBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // create vertex buffer
    const auto vertexBufferSize = cpuMesh.vertices.size() * sizeof(CPUMesh::Vertex);
    buffers.vertexBuffer = createBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    buffers.vertexBufferAddress = getBufferAddress(buffers.vertexBuffer);

    const auto staging = createBuffer(
        vertexBufferSize + indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);

    // copy data
    void* data = staging.info.pMappedData;
    memcpy(data, cpuMesh.vertices.data(), vertexBufferSize);
    memcpy((char*)data + vertexBufferSize, cpuMesh.indices.data(), indexBufferSize);

    executor.immediateSubmit([&](VkCommandBuffer cmd) {
        const auto vertexCopy = VkBufferCopy{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = vertexBufferSize,
        };
        vkCmdCopyBuffer(cmd, staging.buffer, buffers.vertexBuffer.buffer, 1, &vertexCopy);

        const auto indexCopy = VkBufferCopy{
            .srcOffset = vertexBufferSize,
            .dstOffset = 0,
            .size = indexBufferSize,
        };
        vkCmdCopyBuffer(cmd, staging.buffer, buffers.indexBuffer.buffer, 1, &indexCopy);
    });

    destroyBuffer(staging);

    gpuMesh.buffers = buffers;

    if (gpuMesh.hasSkeleton) {
        // create skinning data buffer
        const auto skinningDataSize = cpuMesh.vertices.size() * sizeof(CPUMesh::SkinningData);
        gpuMesh.skinningDataBuffer = createBuffer(
            skinningDataSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
        gpuMesh.skinningDataBufferAddress = getBufferAddress(gpuMesh.skinningDataBuffer);

        const auto staging = createBuffer(
            skinningDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        // copy data
        void* data = staging.info.pMappedData;
        memcpy(data, cpuMesh.skinningData.data(), vertexBufferSize);

        executor.immediateSubmit([&](VkCommandBuffer cmd) {
            const auto vertexCopy = VkBufferCopy{
                .srcOffset = 0,
                .dstOffset = 0,
                .size = skinningDataSize,
            };
            vkCmdCopyBuffer(cmd, staging.buffer, gpuMesh.skinningDataBuffer.buffer, 1, &vertexCopy);
        });

        destroyBuffer(staging);
    }
}

SkinnedMesh Renderer::createSkinnedMeshBuffer(MeshId meshId) const
{
    const auto& mesh = getMesh(meshId);
    SkinnedMesh sm;
    sm.skinnedVertexBuffer = createBuffer(
        mesh.numVertices * sizeof(CPUMesh::Vertex),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    sm.skinnedVertexBufferAddress = getBufferAddress(sm.skinnedVertexBuffer);
    return sm;
}

const GPUMesh& Renderer::getMesh(MeshId id) const
{
    return meshCache.getMesh(id);
}

void Renderer::allocateMaterialDataBuffer()
{
    materialDataBuffer = createBuffer(
        MAX_MATERIALS * sizeof(MaterialData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    vkutil::addDebugLabel(device, materialDataBuffer.buffer, "material data");

    deletionQueue.pushFunction([this](VkDevice) { destroyBuffer(materialDataBuffer); });
}

MaterialId Renderer::addMaterial(Material material)
{
    const auto materialId = materialCache.getFreeMaterialId();
    assert(materialId < MAX_MATERIALS - 1);

    MaterialData* data = (MaterialData*)materialDataBuffer.info.pMappedData;
    data[materialId] = MaterialData{
        .baseColor = material.baseColor,
        .metalRoughnessFactors =
            glm::vec4{material.metallicFactor, material.roughnessFactor, 0.f, 0.f},
    };

    material.materialSet = descriptorAllocator.allocate(device, meshMaterialLayout);

    DescriptorWriter writer;
    writer.writeBuffer(
        0,
        materialDataBuffer.buffer,
        sizeof(MaterialData),
        materialId * sizeof(MaterialData),
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeImage(
        1,
        material.hasDiffuseTexture ? material.diffuseTexture.imageView : whiteTexture.imageView,
        defaultLinearSampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    writer.updateSet(device, material.materialSet);

    materialCache.addMaterial(materialId, std::move(material));

    return materialId;
}

const Material& Renderer::getMaterial(MaterialId id) const
{
    return materialCache.getMaterial(id);
}
