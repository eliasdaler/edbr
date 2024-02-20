#include "Renderer.h"

#include <VkInit.h>
#include <VkPipelines.h>
#include <VkUtil.h>

#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric> // iota

#include <vulkan/vulkan.h>

#include <VkBootstrap.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include <Graphics/FrustumCulling.h>
#include <Graphics/Scene.h>
#include <util/GltfLoader.h>

namespace
{
static constexpr std::uint32_t SCREEN_WIDTH = 1024;
static constexpr std::uint32_t SCREEN_HEIGHT = 768;
static constexpr auto NO_TIMEOUT = std::numeric_limits<std::uint64_t>::max();
}

void Renderer::init()
{
    initVulkan();
    createSwapchain(SCREEN_WIDTH, SCREEN_HEIGHT);
    createCommandBuffers();
    initSyncStructures();
    initImmediateStructures();
    initDescriptors();
    initPipelines();

    initImGui();

    { // init nearest sampler
        auto samplerCreateInfo = VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
        };
        VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &defaultSamplerNearest));
        deletionQueue.pushFunction(
            [this]() { vkDestroySampler(device, defaultSamplerNearest, nullptr); });
    }

    { // create white texture
        std::uint32_t white = 0xFFFFFFFF;
        whiteTexture = createImage(
            (void*)&white,
            VkExtent3D{1, 1, 1},
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT,
            false);

        deletionQueue.pushFunction([this]() { destroyImage(whiteTexture); });
    }

    gradientConstants = ComputePushConstants{
        .data1 = glm::vec4{239.f / 255.f, 157.f / 255.f, 8.f / 255.f, 1},
        .data2 = glm::vec4{85.f / 255.f, 18.f / 255.f, 85.f / 255.f, 1},
    };

    util::LoadContext loadContext{
        .renderer = *this,
        .materialCache = materialCache,
        .meshCache = meshCache,
        .whiteTexture = whiteTexture,
    };

    {
        Scene scene;
        util::SceneLoader loader;
        loader.loadScene(loadContext, scene, "assets/levels/house/house.gltf");
        createEntitiesFromScene(scene);
    }

    {
        Scene scene;
        util::SceneLoader loader;
        loader.loadScene(loadContext, scene, "assets/models/cato.gltf");
        createEntitiesFromScene(scene);

        const glm::vec3 catoPos{1.4f, 0.f, 0.f};
        auto& cato = findEntityByName("Cato");
        cato.transform.position = catoPos;
    }

    {
        static const float zNear = 1.f;
        static const float zFar = 1000.f;
        static const float fovX = glm::radians(45.f);
        static const float aspectRatio = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;

        camera.init(fovX, zNear, zFar, aspectRatio);

        const auto startPos = glm::vec3{8.9f, 4.09f, 8.29f};
        cameraController.setYawPitch(-2.5f, 0.2f);
        camera.setPosition(startPos);
    }
}

void Renderer::initVulkan()
{
    instance = vkb::InstanceBuilder{}
                   .set_app_name("Vulkan app")
                   .request_validation_layers()
                   .use_default_debug_messenger()
                   .require_api_version(1, 3, 0)
                   .build()
                   .value();

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        std::exit(1);
    }

    window = SDL_CreateWindow(
        "Vulkan app",
        // pos
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        // size
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_VULKAN);

    if (!window) {
        std::cout << "Failed to create window. SDL Error: " << SDL_GetError();
        std::exit(1);
    }

    auto res = SDL_Vulkan_CreateSurface(window, instance, &surface);
    if (res != SDL_TRUE) {
        std::cout << "Failed to create Vulkan surface: " << SDL_GetError() << std::endl;
        std::exit(1);
    }

    const auto features12 = VkPhysicalDeviceVulkan12Features{
        .bufferDeviceAddress = true,
    };
    const auto features13 = VkPhysicalDeviceVulkan13Features{
        .synchronization2 = true,
        .dynamicRendering = true,
    };

    physicalDevice = vkb::PhysicalDeviceSelector{instance}
                         .set_minimum_version(1, 3)
                         .set_required_features_12(features12)
                         .set_required_features_13(features13)
                         .set_surface(surface)
                         .select()
                         .value();

    device = vkb::DeviceBuilder{physicalDevice}.build().value();

    graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();

    const auto allocatorInfo = VmaAllocatorCreateInfo{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = physicalDevice,
        .device = device,
        .instance = instance,
    };
    vmaCreateAllocator(&allocatorInfo, &allocator);

    deletionQueue.pushFunction([&]() { vmaDestroyAllocator(allocator); });
}

void Renderer::createSwapchain(std::uint32_t width, std::uint32_t height)
{
    swapchain = vkb::SwapchainBuilder{device}
                    .set_desired_format(VkSurfaceFormatKHR{
                        .format = VK_FORMAT_B8G8R8A8_UNORM,
                        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
                    })
                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                    .set_desired_present_mode(
                        vSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
                    .set_desired_extent(width, height)
                    .build()
                    .value();
    swapchainImages = swapchain.get_images().value();
    swapchainImageViews = swapchain.get_image_views().value();
    swapchainExtent = VkExtent2D{.width = width, .height = height};

    const auto drawImageExtent = VkExtent3D{
        .width = width,
        .height = height,
        .depth = 1,
    };

    { // setup draw image
        drawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        drawImage.extent = drawImageExtent;

        VkImageUsageFlags usages{};
        usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        usages |= VK_IMAGE_USAGE_STORAGE_BIT;
        usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const auto imgCreateInfo =
            vkinit::imageCreateInfo(drawImage.format, usages, drawImageExtent);

        const auto imgAllocInfo = VmaAllocationCreateInfo{
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };

        vmaCreateImage(
            allocator,
            &imgCreateInfo,
            &imgAllocInfo,
            &drawImage.image,
            &drawImage.allocation,
            nullptr);

        const auto imageViewCreateInfo = vkinit::
            imageViewCreateInfo(drawImage.format, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
        VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &drawImage.imageView));

        deletionQueue.pushFunction([this]() { destroyImage(drawImage); });
    }

    { // setup draw image (depth)
        depthImage.format = VK_FORMAT_D32_SFLOAT;
        depthImage.extent = drawImageExtent;

        const auto usages = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        const auto imgCreateInfo =
            vkinit::imageCreateInfo(depthImage.format, usages, drawImageExtent);

        const auto imgAllocInfo = VmaAllocationCreateInfo{
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };

        vmaCreateImage(
            allocator,
            &imgCreateInfo,
            &imgAllocInfo,
            &depthImage.image,
            &depthImage.allocation,
            nullptr);

        const auto imageViewCreateInfo = vkinit::
            imageViewCreateInfo(depthImage.format, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);
        VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &depthImage.imageView));

        deletionQueue.pushFunction([this]() { destroyImage(depthImage); });
    }
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
    const auto fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
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

void Renderer::initImmediateStructures()
{
    const auto poolCreateInfo = vkinit::
        commandPoolCreateInfo(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphicsQueueFamily);
    VK_CHECK(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &immCommandPool));

    const auto cmdAllocInfo = vkinit::commandBufferAllocateInfo(immCommandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &immCommandBuffer));

    deletionQueue.pushFunction([this]() { vkDestroyCommandPool(device, immCommandPool, nullptr); });

    const auto fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &immFence));
    deletionQueue.pushFunction([this]() { vkDestroyFence(device, immFence, nullptr); });
}

void Renderer::initDescriptors()
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
        fd.frameDescriptors = DescriptorAllocatorGrowable{};
        fd.frameDescriptors.init(device, 1000, frame_sizes);

        deletionQueue.pushFunction([&, i]() { frames[i].frameDescriptors.destroyPools(device); });
    }

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

    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        meshMaterialLayout = builder.build(device, VK_SHADER_STAGE_FRAGMENT_BIT);

        deletionQueue.pushFunction(
            [this]() { vkDestroyDescriptorSetLayout(device, meshMaterialLayout, nullptr); });
    }
}

void Renderer::initPipelines()
{
    initBackgroundPipelines();
    initTrianglePipeline();
    initMeshPipeline();
}

void Renderer::initBackgroundPipelines()
{
    const auto pushContant = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(ComputePushConstants),
    };

    const auto layout = VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &drawImageDescriptorLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushContant,
    };

    VK_CHECK(vkCreatePipelineLayout(device, &layout, nullptr, &gradientPipelineLayout));

    VkShaderModule shader{};
    vkutil::loadShaderModule("shaders/gradient.comp.spv", device, &shader);

    const auto pipelineCreateInfo = VkComputePipelineCreateInfo{
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

void Renderer::initTrianglePipeline()
{
    VkShaderModule vertexShader{};
    vkutil::loadShaderModule("shaders/colored_triangle.vert.spv", device, &vertexShader);
    VkShaderModule fragShader{};
    vkutil::loadShaderModule("shaders/colored_triangle.frag.spv", device, &fragShader);

    const auto pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo();
    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &trianglePipelineLayout));

    trianglePipeline = PipelineBuilder{trianglePipelineLayout}
                           .setShaders(vertexShader, fragShader)
                           .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                           .setPolygonMode(VK_POLYGON_MODE_FILL)
                           .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                           .setMultisamplingNone()
                           .disableBlending()
                           .disableDepthTest()
                           .setColorAttachmentFormat(drawImage.format)
                           .setDepthFormat(VK_FORMAT_UNDEFINED)
                           .build(device);

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);

    deletionQueue.pushFunction([this]() {
        vkDestroyPipelineLayout(device, trianglePipelineLayout, nullptr);
        vkDestroyPipeline(device, trianglePipeline, nullptr);
    });
}

void Renderer::initMeshPipeline()
{
    VkShaderModule vertexShader{};
    vkutil::loadShaderModule("shaders/mesh.vert.spv", device, &vertexShader);
    VkShaderModule fragShader{};
    vkutil::loadShaderModule("shaders/mesh.frag.spv", device, &fragShader);

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(GPUDrawPushConstants),
    };

    auto pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &bufferRange;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &meshMaterialLayout;

    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &meshPipelineLayout));

    meshPipeline = PipelineBuilder{meshPipelineLayout}
                       .setShaders(vertexShader, fragShader)
                       .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                       .setPolygonMode(VK_POLYGON_MODE_FILL)
                       .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                       .setMultisamplingNone()
                       .disableBlending()
                       .setColorAttachmentFormat(drawImage.format)
                       .setDepthFormat(depthImage.format)
                       .enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                       .build(device);

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);

    deletionQueue.pushFunction([this]() {
        vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
        vkDestroyPipeline(device, meshPipeline, nullptr);
    });
}

void Renderer::initImGui()
{
    VkDescriptorPoolSize pool_sizes[] =
        {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
         {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
         {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
         {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
         {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    const auto poolInfo = VkDescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1000,
        .poolSizeCount = (std::uint32_t)std::size(pool_sizes),
        .pPoolSizes = pool_sizes,
    };

    VkDescriptorPool imguiPool;
    VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiPool));

    ImGui::CreateContext();

    ImGui_ImplSDL2_InitForVulkan(window);

    auto initInfo = ImGui_ImplVulkan_InitInfo{
        .Instance = instance,
        .PhysicalDevice = physicalDevice,
        .Device = device,
        .QueueFamily = graphicsQueueFamily,
        .Queue = graphicsQueue,
        .DescriptorPool = imguiPool,
        .MinImageCount = 3,
        .ImageCount = 3,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo =
            VkPipelineRenderingCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &swapchain.image_format,
            },
        .CheckVkResultFn = [](VkResult res) { assert(res == VK_SUCCESS); },
    };

    ImGui_ImplVulkan_Init(&initInfo);

    deletionQueue.pushFunction([this, imguiPool]() {
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(device, imguiPool, nullptr);
    });
}

void Renderer::destroyCommandBuffers()
{
    for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        vkDestroyCommandPool(device, frames[i].commandPool, 0);
    }
}

void Renderer::destroySyncStructures()
{
    for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        vkDestroyFence(device, frames[i].renderFence, nullptr);
        vkDestroySemaphore(device, frames[i].swapchainSemaphore, nullptr);
        vkDestroySemaphore(device, frames[i].renderSemaphore, nullptr);
    }
}

AllocatedBuffer Renderer::createBuffer(
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
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = memoryUsage,
    };

    AllocatedBuffer buffer{};
    VK_CHECK(vmaCreateBuffer(
        allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.info));

    return buffer;
}

AllocatedImage Renderer::createImage(
    VkExtent3D size,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap)
{
    AllocatedImage image{
        .extent = size,
        .format = format,
    };

    auto imgInfo = vkinit::imageCreateInfo(format, usage, size);
    if (mipMap) {
        imgInfo.mipLevels =
            static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
    }

    const auto allocInfo = VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    VK_CHECK(
        vmaCreateImage(allocator, &imgInfo, &allocInfo, &image.image, &image.allocation, nullptr));

    auto aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT) {
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    auto viewInfo = vkinit::imageViewCreateInfo(format, image.image, aspectFlag);
    viewInfo.subresourceRange.levelCount = imgInfo.mipLevels;

    VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &image.imageView));

    return image;
}

AllocatedImage Renderer::createImage(
    void* data,
    VkExtent3D size,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap)
{
    size_t data_size = size.depth * size.width * size.height * 4;
    const auto uploadbuffer =
        createBuffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(uploadbuffer.info.pMappedData, data, data_size);

    const auto image = createImage(
        size,
        format,
        usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        mipMap);

    immediateSubmit([&](VkCommandBuffer cmd) {
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
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            .imageExtent = size,
        };

        // copy the buffer into the image
        vkCmdCopyBufferToImage(
            cmd,
            uploadbuffer.buffer,
            image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion);

        vkutil::transitionImage(
            cmd,
            image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    destroyBuffer(uploadbuffer);

    return image;
}

void Renderer::destroyBuffer(const AllocatedBuffer& buffer) const
{
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

void Renderer::destroyImage(const AllocatedImage& image) const
{
    vkDestroyImageView(device, image.imageView, nullptr);
    vmaDestroyImage(allocator, image.image, image.allocation);
}

GPUMeshBuffers Renderer::uploadMesh(
    std::span<const std::uint32_t> indices,
    std::span<const Mesh::Vertex> vertices) const
{
    GPUMeshBuffers mesh;

    // create index buffer
    const auto indexBufferSize = indices.size() * sizeof(std::uint32_t);
    mesh.indexBuffer = createBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // create vertex buffer
    const auto vertexBufferSize = vertices.size() * sizeof(Mesh::Vertex);
    mesh.vertexBuffer = createBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // find the adress of the vertex buffer
    const auto deviceAdressInfo = VkBufferDeviceAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = mesh.vertexBuffer.buffer,
    };
    mesh.vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAdressInfo);

    const auto staging = createBuffer(
        vertexBufferSize + indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);

    // copy data
    void* data = staging.allocation->GetMappedData();
    memcpy(data, vertices.data(), vertexBufferSize);
    memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

    immediateSubmit([&](VkCommandBuffer cmd) {
        const auto vertexCopy = VkBufferCopy{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = vertexBufferSize,
        };
        vkCmdCopyBuffer(cmd, staging.buffer, mesh.vertexBuffer.buffer, 1, &vertexCopy);

        const auto indexCopy = VkBufferCopy{
            .srcOffset = vertexBufferSize,
            .dstOffset = 0,
            .size = indexBufferSize,
        };
        vkCmdCopyBuffer(cmd, staging.buffer, mesh.indexBuffer.buffer, 1, &indexCopy);
    });

    destroyBuffer(staging);

    return mesh;
}

void Renderer::handleInput(float dt)
{
    cameraController.handleInput(camera);
}

void Renderer::update(float dt)
{
    cameraController.update(camera, dt);
    updateEntityTransforms();

    // ImGui::ShowDemoWindow();
    if (displayFPSDelay > 0.f) {
        displayFPSDelay -= dt;
    } else {
        displayFPSDelay = 1.f;
        displayedFPS = avgFPS;
    }

    ImGui::Begin("Debug");
    ImGui::Text("FPS: %d", (int)displayedFPS);
    if (ImGui::Checkbox("VSync", &vSync)) {
        // TODO
    }
    ImGui::Checkbox("Frame limit", &frameLimit);

    const auto cameraPos = camera.getPosition();
    ImGui::Text("Camera pos: %.2f, %.2f, %.2f", cameraPos.x, cameraPos.y, cameraPos.z);
    const auto yaw = cameraController.getYaw();
    const auto pitch = cameraController.getPitch();
    ImGui::Text("Camera rotation: (yaw) %.2f, (pitch) %.2f", yaw, pitch);

    ImGui::End();

    {
        auto glmToArr = [](const glm::vec4& v) { return std::array<float, 4>{v.x, v.y, v.z, v.w}; };
        auto arrToGLM = [](const std::array<float, 4>& v) {
            return glm::vec4{v[0], v[1], v[2], v[3]};
        };

        ImGui::Begin("Gradient");

        auto from = glmToArr(gradientConstants.data1);
        if (ImGui::ColorEdit3("From", from.data())) {
            gradientConstants.data1 = arrToGLM(from);
        }

        auto to = glmToArr(gradientConstants.data2);
        if (ImGui::ColorEdit3("To", to.data())) {
            gradientConstants.data2 = arrToGLM(to);
        }
        ImGui::End();
    }
}

void Renderer::run()
{
    // Fix your timestep! game loop
    const float FPS = 60.f;
    const float dt = 1.f / FPS;

    auto prevTime = std::chrono::high_resolution_clock::now();
    float accumulator = dt; // so that we get at least 1 update before render

    isRunning = true;
    while (isRunning) {
        const auto newTime = std::chrono::high_resolution_clock::now();
        frameTime = std::chrono::duration<float>(newTime - prevTime).count();

        accumulator += frameTime;
        prevTime = newTime;

        // moving average
        float newFPS = 1.f / frameTime;
        avgFPS = std::lerp(avgFPS, newFPS, 0.1f);

        if (accumulator > 10 * dt) { // game stopped for debug
            accumulator = dt;
        }

        while (accumulator >= dt) {
            { // event processing
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    ImGui_ImplSDL2_ProcessEvent(&event);
                    if (event.type == SDL_QUIT) {
                        isRunning = false;
                        return;
                    }
                }
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            // update
            handleInput(dt);
            update(dt);

            accumulator -= dt;

            ImGui::Render();
        }

        generateDrawList();
        draw();

        if (frameLimit) {
            // Delay to not overload the CPU
            const auto now = std::chrono::high_resolution_clock::now();
            const auto frameTime = std::chrono::duration<float>(now - prevTime).count();
            if (dt > frameTime) {
                SDL_Delay(static_cast<std::uint32_t>(dt - frameTime));
            }
        }
    }
}

Renderer::FrameData& Renderer::getCurrentFrame()
{
    return frames[frameNumber % FRAME_OVERLAP];
}

void Renderer::draw()
{
    auto& currentFrame = getCurrentFrame();
    VK_CHECK(vkWaitForFences(device, 1, &currentFrame.renderFence, true, NO_TIMEOUT));
    currentFrame.deletionQueue.flush();

    VK_CHECK(vkResetFences(device, 1, &currentFrame.renderFence));

    drawExtent.width = drawImage.extent.width;
    drawExtent.height = drawImage.extent.height;

    auto& cmd = currentFrame.mainCommandBuffer;
    const auto cmdBeginInfo =
        vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    vkutil::
        transitionImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    drawBackground(cmd);

    vkutil::transitionImage(
        cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkutil::transitionImage(
        cmd, depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    drawGeometry(cmd);

    vkutil::transitionImage(
        cmd,
        drawImage.image,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // get swapchain image
    std::uint32_t swapchainImageIndex{};
    VK_CHECK(vkAcquireNextImageKHR(
        device,
        swapchain,
        NO_TIMEOUT,
        currentFrame.swapchainSemaphore,
        VK_NULL_HANDLE,
        &swapchainImageIndex));
    auto& swapchainImage = swapchainImages[swapchainImageIndex];

    // copy from draw image
    vkutil::transitionImage(
        cmd, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkutil::copyImageToImage(cmd, drawImage.image, swapchainImage, drawExtent, swapchainExtent);

    vkutil::transitionImage(
        cmd,
        swapchainImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    drawImGui(cmd, swapchainImageViews[swapchainImageIndex]);

    // prepare for present
    vkutil::transitionImage(
        cmd,
        swapchainImage,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

    vkCmdPushConstants(
        cmd,
        gradientPipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(ComputePushConstants),
        &gradientConstants);

    vkCmdDispatch(
        cmd, std::ceil(drawExtent.width / 16.f), std::ceil(drawExtent.height / 16.f), 1.f);
}

void Renderer::drawGeometry(VkCommandBuffer cmd)
{
    const auto colorAttachment =
        vkinit::attachmentInfo(drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    const auto depthAttachment =
        vkinit::depthAttachmentInfo(depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    const auto renderInfo = vkinit::renderingInfo(drawExtent, &colorAttachment, &depthAttachment);
    vkCmdBeginRendering(cmd, &renderInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

    const auto viewport = VkViewport{
        .x = 0,
        .y = 0,
        .width = (float)drawExtent.width,
        .height = (float)drawExtent.height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    const auto scissor = VkRect2D{
        .offset = {},
        .extent = drawExtent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    auto prevMaterialIdx = NULL_MATERIAL_ID;
    auto prevMeshId = NULL_MESH_ID;

    const auto frustum = edge::createFrustumFromCamera(camera);

    for (const auto& dcIdx : sortedDrawCommands) {
        const auto& dc = drawCommands[dcIdx];
        if (!edge::isInFrustum(frustum, dc.worldBoundingSphere)) {
            continue;
        }

        if (dc.mesh.materialId != prevMaterialIdx) {
            prevMaterialIdx = dc.mesh.materialId;

            const auto& material = materialCache.getMaterial(dc.mesh.materialId);
            auto imageSet = getCurrentFrame().frameDescriptors.allocate(device, meshMaterialLayout);
            {
                DescriptorWriter writer;
                writer.writeImage(
                    0,
                    material.diffuseTexture.imageView,
                    defaultSamplerNearest,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

                writer.updateSet(device, imageSet);
            }
            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                meshPipelineLayout,
                0,
                1,
                &imageSet,
                0,
                nullptr);
        }

        if (dc.meshId != prevMeshId) {
            prevMeshId = dc.meshId;
            vkCmdBindIndexBuffer(cmd, dc.mesh.buffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        }

        const auto pushConstants = GPUDrawPushConstants{
            .worldMatrix = camera.getViewProj() * dc.transformMatrix,
            .vertexBuffer = dc.mesh.buffers.vertexBufferAddress,
        };
        vkCmdPushConstants(
            cmd,
            meshPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(GPUDrawPushConstants),
            &pushConstants);

        vkCmdDrawIndexed(cmd, dc.mesh.numIndices, 1, 0, 0, 0);
    }

    vkCmdEndRendering(cmd);
}

void Renderer::drawImGui(VkCommandBuffer cmd, VkImageView targetImageView)
{
    const auto colorAttachment =
        vkinit::attachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    const auto renderInfo = vkinit::renderingInfo(swapchainExtent, &colorAttachment, nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}

void Renderer::cleanup()
{
    vkDeviceWaitIdle(device);

    meshCache.cleanup(*this);
    materialCache.cleanup(*this);

    deletionQueue.flush();

    destroyCommandBuffers();
    destroySyncStructures();

    descriptorAllocator.destroyPool(device);

    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    swapchainImageViews.clear();

    vkb::destroy_swapchain(swapchain);
    vkb::destroy_surface(instance, surface);
    vkb::destroy_device(device);
    vkb::destroy_instance(instance);

    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Renderer::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const
{
    VK_CHECK(vkResetFences(device, 1, &immFence));
    VK_CHECK(vkResetCommandBuffer(immCommandBuffer, 0));

    auto cmd = immCommandBuffer;
    const auto cmdBeginInfo =
        vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    function(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    const auto cmdinfo = vkinit::commandBufferSubmitInfo(cmd);
    const auto submit = vkinit::submitInfo(&cmdinfo, nullptr, nullptr);

    VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, immFence));

    VK_CHECK(vkWaitForFences(device, 1, &immFence, true, NO_TIMEOUT));
}

void Renderer::createEntitiesFromScene(const Scene& scene)
{
    for (const auto& nodePtr : scene.nodes) {
        if (nodePtr) {
            createEntitiesFromNode(scene, *nodePtr);
        }
    }
}

Renderer::EntityId Renderer::createEntitiesFromNode(
    const Scene& scene,
    const SceneNode& node,
    EntityId parentId)
{
    auto& e = makeNewEntity();
    e.tag = node.name;

    // transform
    {
        e.transform = node.transform;
        if (parentId == NULL_ENTITY_ID) {
            e.worldTransform = e.transform.asMatrix();
        } else {
            const auto& parent = entities[parentId];
            e.worldTransform = parent->worldTransform * node.transform.asMatrix();
        }
    }

    { // mesh
        e.meshes = scene.meshes[node.meshIndex].primitives;
        // TODO: skeleton
    }

    { // hierarchy
        e.parentId = parentId;
        for (const auto& childPtr : node.children) {
            if (childPtr) {
                const auto childId = createEntitiesFromNode(scene, *childPtr, e.id);
                e.children.push_back(childId);
            }
        }
    }

    return e.id;
}

Renderer::Entity& Renderer::makeNewEntity()
{
    entities.push_back(std::make_unique<Entity>());
    auto& e = *entities.back();
    e.id = entities.size() - 1;
    return e;
}

Renderer::Entity& Renderer::findEntityByName(std::string_view name) const
{
    for (const auto& ePtr : entities) {
        if (ePtr->tag == name) {
            return *ePtr;
        }
    }

    throw std::runtime_error(std::string{"failed to find entity with name "} + std::string{name});
}

void Renderer::updateEntityTransforms()
{
    const auto I = glm::mat4{1.f};
    for (auto& ePtr : entities) {
        auto& e = *ePtr;
        if (e.parentId == NULL_MESH_ID) {
            updateEntityTransforms(e, I);
        }
    }
}

void Renderer::updateEntityTransforms(Entity& e, const glm::mat4& parentWorldTransform)
{
    const auto prevTransform = e.worldTransform;
    e.worldTransform = parentWorldTransform * e.transform.asMatrix();
    if (e.worldTransform == prevTransform) {
        return;
    }

    for (const auto& childId : e.children) {
        auto& child = *entities[childId];
        updateEntityTransforms(child, e.worldTransform);
    }
}

void Renderer::generateDrawList()
{
    drawCommands.clear();

    for (const auto& ePtr : entities) {
        const auto& e = *ePtr;
        const auto transformMat = e.worldTransform;

        for (std::size_t meshIdx = 0; meshIdx < e.meshes.size(); ++meshIdx) {
            const auto& mesh = meshCache.getMesh(e.meshes[meshIdx]);
            // TODO: draw frustum culling here
            const auto& material = materialCache.getMaterial(mesh.materialId);

            const auto worldBoundingSphere =
                edge::calculateBoundingSphereWorld(transformMat, mesh.boundingSphere, false);

            drawCommands.push_back(DrawCommand{
                .mesh = mesh,
                .meshId = e.meshes[meshIdx],
                .transformMatrix = transformMat,
                .worldBoundingSphere = worldBoundingSphere,
            });
        }
    }

    sortDrawList();
}

void Renderer::sortDrawList()
{
    sortedDrawCommands.clear();
    sortedDrawCommands.resize(drawCommands.size());
    std::iota(sortedDrawCommands.begin(), sortedDrawCommands.end(), 0);

    std::sort(
        sortedDrawCommands.begin(),
        sortedDrawCommands.end(),
        [this](const auto& i1, const auto& i2) {
            const auto& dc1 = drawCommands[i1];
            const auto& dc2 = drawCommands[i2];
            if (dc1.mesh.materialId == dc2.mesh.materialId) {
                return dc1.meshId < dc2.meshId;
            }
            return dc1.mesh.materialId < dc2.mesh.materialId;
        });
}
