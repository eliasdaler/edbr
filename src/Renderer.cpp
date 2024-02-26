#include "Renderer.h"

#include <VkInit.h>
#include <VkPipelines.h>
#include <VkUtil.h>

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric> // iota

#include <vulkan/vulkan.h>

#include <VkBootstrap.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <vk_mem_alloc.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include <Graphics/FrustumCulling.h>
#include <Graphics/Scene.h>
#include <Graphics/ShadowMapping.h>
#include <Graphics/Skeleton.h>

#include <util/GltfLoader.h>
#include <util/ImageLoader.h>

namespace
{
static constexpr auto NO_TIMEOUT = std::numeric_limits<std::uint64_t>::max();
}

void Renderer::init(SDL_Window* window, bool vSync)
{
    initVulkan(window);

    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    createSwapchain((std::uint32_t)w, (std::uint32_t)h, vSync);
    createCommandBuffers();
    initSyncStructures();
    initImmediateStructures();
    initDescriptorAllocators();
    initDescriptors();
    initPipelines();
    initSamplers();
    initDefaultTextures();

    allocateMaterialDataBuffer(1000);

    initImGui(window);

    initCSMData();

    gradientConstants = ComputePushConstants{
        .data1 = glm::vec4{239.f / 255.f, 157.f / 255.f, 8.f / 255.f, 1},
        .data2 = glm::vec4{85.f / 255.f, 18.f / 255.f, 85.f / 255.f, 1},
    };
}

void Renderer::initVulkan(SDL_Window* window)
{
    instance = vkb::InstanceBuilder{}
                   .set_app_name("Vulkan app")
                   .request_validation_layers()
                   .use_default_debug_messenger()
                   .require_api_version(1, 3, 0)
                   .build()
                   .value();

    loadExtensionFunctions();

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

    std::vector<const char*> enabledExtensions =
        {VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME};
    VkDeviceCreateInfo deviceInfo = {};
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

void Renderer::loadExtensionFunctions()
{
    // FIXME: use volk
    pfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)
        vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
    pfnQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)
        vkGetInstanceProcAddr(instance, "vkQueueBeginDebugUtilsLabelEXT");
    pfnQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)
        vkGetInstanceProcAddr(instance, "vkQueueEndDebugUtilsLabelEXT");
    pfnCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)
        vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
    pfnCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)
        vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
    pfnCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)
        vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
}

void Renderer::createSwapchain(std::uint32_t width, std::uint32_t height, bool vSync)
{
    // vSync = false;
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

void Renderer::initImmediateStructures()
{
    const auto poolCreateInfo = vkinit::
        commandPoolCreateInfo(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphicsQueueFamily);
    VK_CHECK(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &immCommandPool));

    const auto cmdAllocInfo = vkinit::commandBufferAllocateInfo(immCommandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &immCommandBuffer));

    deletionQueue.pushFunction([this]() { vkDestroyCommandPool(device, immCommandPool, nullptr); });

    const auto fenceCreateInfo = VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &immFence));
    deletionQueue.pushFunction([this]() { vkDestroyFence(device, immFence, nullptr); });
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

        deletionQueue.pushFunction([&, i]() { frames[i].frameDescriptors.destroyPools(device); });
    }

    const auto sizes = std::vector<DescriptorAllocatorGrowable::PoolSizeRatio>{
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
    };

    descriptorAllocator.init(device, 10, sizes);
}

void Renderer::initDescriptors()
{
    const auto drawImageBindings = std::array<vkutil::DescriptorLayoutBinding, 1>{
        {0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
    };
    drawImageDescriptorLayout =
        vkutil::buildDescriptorSetLayout(device, VK_SHADER_STAGE_COMPUTE_BIT, drawImageBindings);

    deletionQueue.pushFunction(
        [this]() { vkDestroyDescriptorSetLayout(device, drawImageDescriptorLayout, nullptr); });

    { // write descriptors
        drawImageDescriptors = descriptorAllocator.allocate(device, drawImageDescriptorLayout);
        DescriptorWriter writer;
        writer.writeImage(
            0,
            drawImage.imageView,
            VK_NULL_HANDLE,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(device, drawImageDescriptors);
    }

    const auto sceneDataBindings = std::array<vkutil::DescriptorLayoutBinding, 2>{{
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    }};
    sceneDataDescriptorLayout = vkutil::buildDescriptorSetLayout(
        device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sceneDataBindings);

    deletionQueue.pushFunction(
        [this]() { vkDestroyDescriptorSetLayout(device, sceneDataDescriptorLayout, nullptr); });

    const auto meshMaterialBindings = std::array<vkutil::DescriptorLayoutBinding, 2>{{
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    }};
    meshMaterialLayout = vkutil::
        buildDescriptorSetLayout(device, VK_SHADER_STAGE_FRAGMENT_BIT, meshMaterialBindings);

    deletionQueue.pushFunction(
        [this]() { vkDestroyDescriptorSetLayout(device, meshMaterialLayout, nullptr); });
}

void Renderer::allocateMaterialDataBuffer(std::size_t numMaterials)
{
    materialDataBuffer = createBuffer(
        numMaterials * sizeof(MaterialData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    addDebugLabel(materialDataBuffer, "material data");

    deletionQueue.pushFunction([this]() { destroyBuffer(materialDataBuffer); });
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
            [this]() { vkDestroySampler(device, defaultNearestSampler, nullptr); });
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
            [this]() { vkDestroySampler(device, defaultLinearSampler, nullptr); });
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
            [this]() { vkDestroySampler(device, defaultShadowMapSampler, nullptr); });
    }
}

void Renderer::initDefaultTextures()
{
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
}

void Renderer::initPipelines()
{
    initBackgroundPipelines();
    initSkinningPipeline();
    initTrianglePipeline();
    initMeshPipeline();
    initMeshDepthOnlyPipeline();
}

void Renderer::initBackgroundPipelines()
{
    const auto pushConstant = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(ComputePushConstants),
    };

    const auto pushConstants = std::array{pushConstant};
    const auto layouts = std::array{drawImageDescriptorLayout};
    gradientPipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstants);

    const auto shader = vkutil::loadShaderModule("shaders/gradient.comp.spv", device);
    addDebugLabel(shader, "gradient");

    gradientPipeline =
        ComputePipelineBuilder{gradientPipelineLayout}.setShader(shader).build(device);

    vkDestroyShaderModule(device, shader, nullptr);

    deletionQueue.pushFunction([this]() {
        vkDestroyPipelineLayout(device, gradientPipelineLayout, nullptr);
        vkDestroyPipeline(device, gradientPipeline, nullptr);
    });
}

void Renderer::initSkinningPipeline()
{
    const auto pushConstant = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(SkinningPushConstants),
    };

    const auto pushConstants = std::array{pushConstant};
    skinningPipelineLayout = vkutil::createPipelineLayout(device, {}, pushConstants);

    const auto shader = vkutil::loadShaderModule("shaders/skinning.comp.spv", device);
    addDebugLabel(shader, "skinning");

    skinningPipeline =
        ComputePipelineBuilder{skinningPipelineLayout}.setShader(shader).build(device);

    vkDestroyShaderModule(device, shader, nullptr);

    for (std::size_t i = 0; i < FRAME_OVERLAP; ++i) {
        auto& jointMatricesBuffer = frames[i].jointMatricesBuffer;
        jointMatricesBuffer.capacity = MAX_JOINT_MATRICES;
        jointMatricesBuffer.buffer = createBuffer(
            MAX_JOINT_MATRICES * sizeof(glm::mat4),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);
        frames[i].jointMatricesBufferAddress = getBufferAddress(jointMatricesBuffer.buffer);

        deletionQueue.pushFunction(
            [this, &jointMatricesBuffer]() { destroyBuffer(jointMatricesBuffer.buffer); });
    }

    deletionQueue.pushFunction([this]() {
        vkDestroyPipelineLayout(device, skinningPipelineLayout, nullptr);
        vkDestroyPipeline(device, skinningPipeline, nullptr);
    });
}

void Renderer::initTrianglePipeline()
{
    const auto vertexShader = vkutil::loadShaderModule("shaders/colored_triangle.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/colored_triangle.frag.spv", device);

    trianglePipelineLayout = vkutil::createPipelineLayout(device);

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
    const auto vertexShader = vkutil::loadShaderModule("shaders/mesh.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/mesh.frag.spv", device);

    addDebugLabel(vertexShader, "mesh.vert");
    addDebugLabel(vertexShader, "mesh.frag");

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(GPUDrawPushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts = std::array{sceneDataDescriptorLayout, meshMaterialLayout};
    meshPipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);

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
    addDebugLabel(meshPipeline, "mesh pipeline");

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);

    deletionQueue.pushFunction([this]() {
        vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
        vkDestroyPipeline(device, meshPipeline, nullptr);
    });
}

void Renderer::initMeshDepthOnlyPipeline()
{
    const auto vertexShader = vkutil::loadShaderModule("shaders/mesh_depth_only.vert.spv", device);

    addDebugLabel(vertexShader, "mesh_depth_only.vert");

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(DepthOnlyPushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts = std::array{sceneDataDescriptorLayout, meshMaterialLayout};
    meshDepthOnlyPipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);

    meshDepthOnlyPipeline = PipelineBuilder{meshDepthOnlyPipelineLayout}
                                .setShaders(vertexShader, VK_NULL_HANDLE)
                                .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                .setPolygonMode(VK_POLYGON_MODE_FILL)
                                .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                                .setMultisamplingNone()
                                .disableBlending()
                                .setDepthFormat(depthImage.format)
                                .enableDepthClamp()
                                .enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                                .build(device);
    addDebugLabel(meshDepthOnlyPipeline, "mesh depth only pipeline");

    vkDestroyShaderModule(device, vertexShader, nullptr);

    deletionQueue.pushFunction([this]() {
        vkDestroyPipelineLayout(device, meshDepthOnlyPipelineLayout, nullptr);
        vkDestroyPipeline(device, meshDepthOnlyPipeline, nullptr);
    });
}

void Renderer::initImGui(SDL_Window* window)
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

void Renderer::initCSMData()
{
    // meshDepthOnly pipeline uses depthImage.format for depth
    assert(depthImage.format == VK_FORMAT_D32_SFLOAT);

    csmShadowMap = createImage(
        VkExtent3D{(std::uint32_t)shadowMapTextureSize, (std::uint32_t)shadowMapTextureSize, 1},
        NUM_SHADOW_CASCADES,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        false);
    addDebugLabel(csmShadowMap, "CSM shadow map");
    addDebugLabel(csmShadowMap.imageView, "CSM shadow map view");

    for (int i = 0; i < NUM_SHADOW_CASCADES; ++i) {
        VkImageView imageView;
        const auto createInfo = VkImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = csmShadowMap.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = csmShadowMap.format,
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = (std::uint32_t)i,
                    .layerCount = 1,
                },
        };
        VK_CHECK(vkCreateImageView(device, &createInfo, nullptr, &csmShadowMapViews[i]));
        addDebugLabel(csmShadowMapViews[i], "CSM shadow map view");
    }

    deletionQueue.pushFunction([this]() {
        destroyImage(csmShadowMap);
        for (int i = 0; i < NUM_SHADOW_CASCADES; ++i) {
            vkDestroyImageView(device, csmShadowMapViews[i], nullptr);
        }
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

[[nodiscard]] VkDeviceAddress Renderer::getBufferAddress(const AllocatedBuffer& buffer) const
{
    const auto deviceAdressInfo = VkBufferDeviceAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer.buffer,
    };
    return vkGetBufferDeviceAddress(device, &deviceAdressInfo);
}

AllocatedImage Renderer::createImage(
    VkExtent3D size,
    std::uint32_t numLayers,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap)
{
    std::uint32_t mipLevels = 1;
    if (mipMap) {
        mipLevels =
            static_cast<std::uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) +
            1;
    }

    const auto imgInfo = VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = size,
        .mipLevels = mipLevels,
        .arrayLayers = numLayers,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
    };
    const auto allocInfo = VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    auto image = AllocatedImage{
        .extent = size,
        .format = format,
        .mipLevels = mipLevels,
    };
    VK_CHECK(
        vmaCreateImage(allocator, &imgInfo, &allocInfo, &image.image, &image.allocation, nullptr));

    // create view
    auto aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT) {
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    const auto viewCreateInfo = VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.image,
        .viewType = numLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        .format = format,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = aspectFlag,
                .baseMipLevel = 0,
                .levelCount = mipLevels,
                .baseArrayLayer = 0,
                .layerCount = numLayers,
            },
    };

    VK_CHECK(vkCreateImageView(device, &viewCreateInfo, nullptr, &image.imageView));

    return image;
}

AllocatedImage Renderer::createImage(
    void* data,
    VkExtent3D size,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap)
{
    const auto data_size = size.depth * size.width * size.height * 4;
    const auto uploadbuffer =
        createBuffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(uploadbuffer.info.pMappedData, data, data_size);

    const auto image = createImage(
        size,
        1,
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

        vkCmdCopyBufferToImage(
            cmd,
            uploadbuffer.buffer,
            image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion);

        if (mipMap) {
            vkutil::generateMipmaps(
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

    destroyBuffer(uploadbuffer);

    return image;
}

AllocatedImage Renderer::loadImageFromFile(
    const std::filesystem::path& path,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap)
{
    auto data = util::loadImage(path);
    assert(data.pixels);

    const auto image = createImage(
        data.pixels,
        VkExtent3D{
            .width = (std::uint32_t)data.width,
            .height = (std::uint32_t)data.height,
            .depth = 1,
        },
        format,
        usage,
        mipMap);

    addDebugLabel(image, path.c_str());

    return image;
}

void Renderer::addDebugLabel(const AllocatedImage& image, const char* label)
{
    assert(pfnSetDebugUtilsObjectNameEXT);
    const auto nameInfo = VkDebugUtilsObjectNameInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_IMAGE,
        .objectHandle = (std::uint64_t)image.image,
        .pObjectName = label,
    };
    pfnSetDebugUtilsObjectNameEXT(device, &nameInfo);
}

void Renderer::addDebugLabel(VkImageView imageView, const char* label)
{
    assert(pfnSetDebugUtilsObjectNameEXT);
    const auto nameInfo = VkDebugUtilsObjectNameInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
        .objectHandle = (std::uint64_t)imageView,
        .pObjectName = label,
    };
    pfnSetDebugUtilsObjectNameEXT(device, &nameInfo);
}

void Renderer::addDebugLabel(const VkShaderModule& shader, const char* label)
{
    assert(pfnSetDebugUtilsObjectNameEXT);
    const auto nameInfo = VkDebugUtilsObjectNameInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_SHADER_MODULE,
        .objectHandle = (std::uint64_t)shader,
        .pObjectName = label,
    };
    pfnSetDebugUtilsObjectNameEXT(device, &nameInfo);
}

void Renderer::addDebugLabel(const VkPipeline& pipeline, const char* label)
{
    assert(pfnSetDebugUtilsObjectNameEXT);
    const auto nameInfo = VkDebugUtilsObjectNameInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_PIPELINE,
        .objectHandle = (std::uint64_t)pipeline,
        .pObjectName = label,
    };
    pfnSetDebugUtilsObjectNameEXT(device, &nameInfo);
}

void Renderer::addDebugLabel(const AllocatedBuffer& buffer, const char* label)
{
    assert(pfnSetDebugUtilsObjectNameEXT);
    const auto nameInfo = VkDebugUtilsObjectNameInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = (std::uint64_t)buffer.buffer,
        .pObjectName = label,
    };
    pfnSetDebugUtilsObjectNameEXT(device, &nameInfo);
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

void Renderer::beginCmdLabel(VkCommandBuffer cmd, const char* label)
{
    const VkDebugUtilsLabelEXT houseLabel = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = label,
        .color = {1.0f, 1.0f, 1.0f, 1.0f},
    };

    assert(pfnCmdBeginDebugUtilsLabelEXT);
    pfnCmdBeginDebugUtilsLabelEXT(cmd, &houseLabel);
}

void Renderer::endCmdLabel(VkCommandBuffer cmd)
{
    assert(pfnCmdEndDebugUtilsLabelEXT);
    pfnCmdEndDebugUtilsLabelEXT(cmd);
}

VkDescriptorSet Renderer::writeMaterialData(MaterialId id, const Material& material)
{
    MaterialData* data = (MaterialData*)materialDataBuffer.info.pMappedData;
    data[id] = MaterialData{
        .baseColor = material.baseColor,
        .metalRoughnessFactors =
            glm::vec4{material.metallicFactor, material.roughnessFactor, 0.f, 0.f},
    };

    const auto set = descriptorAllocator.allocate(device, meshMaterialLayout);

    DescriptorWriter writer;
    writer.writeBuffer(
        0,
        materialDataBuffer.buffer,
        sizeof(MaterialData),
        id * sizeof(MaterialData),
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeImage(
        1,
        material.diffuseTexture.imageView,
        defaultLinearSampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    writer.updateSet(device, set);

    return set;
}

void Renderer::uploadMesh(const Mesh& cpuMesh, GPUMesh& mesh) const
{
    GPUMeshBuffers buffers;

    // create index buffer
    const auto indexBufferSize = cpuMesh.indices.size() * sizeof(std::uint32_t);
    buffers.indexBuffer = createBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // create vertex buffer
    const auto vertexBufferSize = cpuMesh.vertices.size() * sizeof(Mesh::Vertex);
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

    immediateSubmit([&](VkCommandBuffer cmd) {
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

    mesh.buffers = buffers;
    mesh.numVertices = cpuMesh.vertices.size();
    mesh.numIndices = cpuMesh.indices.size();

    if (mesh.hasSkeleton) {
        // create skinning data buffer
        const auto skinningDataSize = cpuMesh.vertices.size() * sizeof(Mesh::SkinningData);
        mesh.skinningDataBuffer = createBuffer(
            skinningDataSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
        mesh.skinningDataBufferAddress = getBufferAddress(mesh.skinningDataBuffer);

        const auto staging = createBuffer(
            skinningDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        // copy data
        void* data = staging.info.pMappedData;
        memcpy(data, cpuMesh.skinningData.data(), vertexBufferSize);

        immediateSubmit([&](VkCommandBuffer cmd) {
            const auto vertexCopy = VkBufferCopy{
                .srcOffset = 0,
                .dstOffset = 0,
                .size = skinningDataSize,
            };
            vkCmdCopyBuffer(cmd, staging.buffer, mesh.skinningDataBuffer.buffer, 1, &vertexCopy);
        });

        destroyBuffer(staging);
    }
}

SkinnedMesh Renderer::createSkinnedMeshBuffer(MeshId meshId) const
{
    const auto& mesh = meshCache.getMesh(meshId);
    SkinnedMesh sm;
    sm.skinnedVertexBuffer = createBuffer(
        mesh.numVertices * sizeof(Mesh::Vertex),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    sm.skinnedVertexBufferAddress = getBufferAddress(sm.skinnedVertexBuffer);
    return sm;
}

void Renderer::updateDevTools(float dt)
{
    auto glmToArr = [](const glm::vec4& v) { return std::array<float, 4>{v.x, v.y, v.z, v.w}; };
    auto arrToGLM = [](const std::array<float, 4>& v) { return glm::vec4{v[0], v[1], v[2], v[3]}; };

    {
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

Renderer::FrameData& Renderer::getCurrentFrame()
{
    return frames[frameNumber % FRAME_OVERLAP];
}

void Renderer::draw(const Camera& camera)
{
    auto& currentFrame = getCurrentFrame();
    VK_CHECK(vkWaitForFences(device, 1, &currentFrame.renderFence, true, NO_TIMEOUT));
    currentFrame.deletionQueue.flush();

    VK_CHECK(vkResetFences(device, 1, &currentFrame.renderFence));

    drawExtent.width = drawImage.extent.width;
    drawExtent.height = drawImage.extent.height;

    const auto& cmd = currentFrame.mainCommandBuffer;
    const auto cmdBeginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    vkutil::
        transitionImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    beginCmdLabel(cmd, "Skinning");
    doSkinning(cmd);
    endCmdLabel(cmd);

    beginCmdLabel(cmd, "Draw CSM");
    drawShadowMaps(cmd, camera);
    endCmdLabel(cmd);

    beginCmdLabel(cmd, "Draw background");
    drawBackground(cmd);
    endCmdLabel(cmd);

    vkutil::transitionImage(
        cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkutil::transitionImage(
        cmd, depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    beginCmdLabel(cmd, "Draw geometry");
    drawGeometry(cmd, camera);
    endCmdLabel(cmd);

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

    beginCmdLabel(cmd, "Draw Dear ImGui");
    drawImGui(cmd, swapchainImageViews[swapchainImageIndex]);
    endCmdLabel(cmd);

    // prepare for present
    vkutil::transitionImage(
        cmd,
        swapchainImage,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));

    { // submit
        const auto submitInfo = VkCommandBufferSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = cmd,
        };
        const auto waitInfo = vkinit::semaphoreSubmitInfo(
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, currentFrame.swapchainSemaphore);
        const auto signalInfo = vkinit::
            semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, currentFrame.renderSemaphore);

        const auto submit = vkinit::submitInfo(&submitInfo, &signalInfo, &waitInfo);
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

void Renderer::doSkinning(VkCommandBuffer cmd)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, skinningPipeline);

    for (const auto& dc : drawCommands) {
        if (!dc.skinnedMesh) {
            continue;
        }

        const auto& mesh = meshCache.getMesh(dc.meshId);
        assert(mesh.hasSkeleton);
        assert(dc.skinnedMesh);

        const auto cs = SkinningPushConstants{
            .jointMatricesBuffer = getCurrentFrame().jointMatricesBufferAddress,
            .jointMatricesStartIndex = dc.jointMatricesStartIndex,
            .numVertices = mesh.numVertices,
            .inputBuffer = mesh.buffers.vertexBufferAddress,
            .skinningData = mesh.skinningDataBufferAddress,
            .outputBuffer = dc.skinnedMesh->skinnedVertexBufferAddress,
        };
        vkCmdPushConstants(
            cmd,
            skinningPipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(SkinningPushConstants),
            &cs);

        static const auto workgroupSize = 256;
        vkCmdDispatch(cmd, std::ceil(mesh.numVertices / (float)workgroupSize), 1, 1);
    };

    const auto memoryBarrier = VkMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT_KHR,
        .dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR,
    };

    const auto dependencyInfo = VkDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &memoryBarrier,
    };

    vkCmdPipelineBarrier2(cmd, &dependencyInfo);
}

void Renderer::drawShadowMaps(VkCommandBuffer cmd, const Camera& camera)
{
    vkutil::transitionImage(
        cmd,
        csmShadowMap.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    std::array<float, NUM_SHADOW_CASCADES> percents = {0.3f, 0.8f, 1.f};
    if (camera.getZFar() > 100.f) {
        percents = {0.01f, 0.04f, 0.15f};
    }

    std::array<float, NUM_SHADOW_CASCADES> cascadeFarPlaneZs{};
    std::array<glm::mat4, NUM_SHADOW_CASCADES> csmLightSpaceTMs{};

    for (int i = 0; i < NUM_SHADOW_CASCADES; ++i) {
        float zNear = i == 0 ? camera.getZNear() : camera.getZNear() * percents[i - 1];
        float zFar = camera.getZFar() * percents[i];
        cascadeFarPlaneZs[i] = zFar;

        // create subfustrum by copying everything about the main camera,
        // but changing zFar
        Camera subFrustumCamera;
        subFrustumCamera.setPosition(camera.getPosition());
        subFrustumCamera.setHeading(camera.getHeading());
        subFrustumCamera.init(camera.getFOVX(), zNear, zFar, 1.f);
        // NOTE: this camera doesn't use inverse depth because otherwise
        // calculateFrustumCornersWorldSpace doesn't work properly

        const auto corners =
            edge::calculateFrustumCornersWorldSpace(subFrustumCamera.getViewProj());
        const auto csmCamera = calculateCSMCamera(
            corners, glm::vec3{sceneData.sunlightDirection}, shadowMapTextureSize);
        csmLightSpaceTMs[i] = csmCamera.getViewProj();

        const auto depthAttachment = vkinit::
            depthAttachmentInfo(csmShadowMapViews[i], VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        const auto renderInfo = vkinit::renderingInfo(
            VkExtent2D{(std::uint32_t)shadowMapTextureSize, (std::uint32_t)shadowMapTextureSize},
            nullptr,
            &depthAttachment);
        vkCmdBeginRendering(cmd, &renderInfo);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshDepthOnlyPipeline);

        const auto viewport = VkViewport{
            .x = 0,
            .y = 0,
            .width = shadowMapTextureSize,
            .height = shadowMapTextureSize,
            .minDepth = 0.f,
            .maxDepth = 1.f,
        };
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        const auto scissor = VkRect2D{
            .offset = {},
            .extent = {(std::uint32_t)shadowMapTextureSize, (std::uint32_t)shadowMapTextureSize},
        };
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        const auto frustum = edge::createFrustumFromCamera(csmCamera);
        auto prevMeshId = NULL_MESH_ID;

        // FIXME: this is sorted by material first so might be not the most efficient
        // we can store by mesh here and do many indexed draws here potentially
        for (const auto& dcIdx : sortedDrawCommands) {
            const auto& dc = drawCommands[dcIdx];
            // hack: don't cull big objects, because shadows from them might disappear
            if (dc.worldBoundingSphere.radius < 2.f &&
                !edge::isInFrustum(frustum, dc.worldBoundingSphere)) {
                continue;
            }

            const auto& mesh = meshCache.getMesh(dc.meshId);

            if (dc.meshId != prevMeshId) {
                prevMeshId = dc.meshId;
                vkCmdBindIndexBuffer(cmd, mesh.buffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            }

            const auto pushConstants = DepthOnlyPushConstants{
                .mvp = csmLightSpaceTMs[i] * dc.transformMatrix,
                .vertexBuffer = dc.skinnedMesh ? dc.skinnedMesh->skinnedVertexBufferAddress :
                                                 mesh.buffers.vertexBufferAddress,
            };
            vkCmdPushConstants(
                cmd,
                meshDepthOnlyPipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(DepthOnlyPushConstants),
                &pushConstants);

            vkCmdDrawIndexed(cmd, mesh.numIndices, 1, 0, 0, 0);
        }

        vkCmdEndRendering(cmd);
    }

    // FIXME: would be better to put it somewhere else
    // otherwise this function does unexpected changes to sceneData?
    sceneData.cascadeFarPlaneZs = glm::vec4{
        cascadeFarPlaneZs[0],
        cascadeFarPlaneZs[1],
        cascadeFarPlaneZs[2],
        0.f,
    };
    sceneData.csmLightSpaceTMs = csmLightSpaceTMs;

    const auto memoryBarrier = VkMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
    };

    const auto dependencyInfo = VkDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &memoryBarrier,
    };

    vkCmdPipelineBarrier2(cmd, &dependencyInfo);

    vkutil::transitionImage(
        cmd,
        csmShadowMap.image,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL);
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

void Renderer::drawGeometry(VkCommandBuffer cmd, const Camera& camera)
{
    const auto sceneDescriptor = uploadSceneData();

    const auto colorAttachment =
        vkinit::attachmentInfo(drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    const auto depthAttachment =
        vkinit::depthAttachmentInfo(depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    const auto renderInfo = vkinit::renderingInfo(drawExtent, &colorAttachment, &depthAttachment);
    vkCmdBeginRendering(cmd, &renderInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        meshPipelineLayout,
        0,
        1,
        &sceneDescriptor,
        0,
        nullptr);

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

        const auto& mesh = meshCache.getMesh(dc.meshId);
        if (mesh.materialId != prevMaterialIdx) {
            prevMaterialIdx = mesh.materialId;

            const auto& material = materialCache.getMaterial(mesh.materialId);
            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                meshPipelineLayout,
                1,
                1,
                &material.materialSet,
                0,
                nullptr);
        }

        if (dc.meshId != prevMeshId) {
            prevMeshId = dc.meshId;
            vkCmdBindIndexBuffer(cmd, mesh.buffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        }

        const auto pushConstants = GPUDrawPushConstants{
            .transform = dc.transformMatrix,
            .vertexBuffer = dc.skinnedMesh ? dc.skinnedMesh->skinnedVertexBufferAddress :
                                             mesh.buffers.vertexBufferAddress,
        };
        vkCmdPushConstants(
            cmd,
            meshPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(GPUDrawPushConstants),
            &pushConstants);

        vkCmdDrawIndexed(cmd, mesh.numIndices, 1, 0, 0, 0);
    }

    vkCmdEndRendering(cmd);
}

VkDescriptorSet Renderer::uploadSceneData()
{
    auto& currentFrame = getCurrentFrame();

    const auto gpuSceneDataBuffer = createBuffer(
        sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    addDebugLabel(gpuSceneDataBuffer, "scene data");

    currentFrame.deletionQueue.pushFunction(
        [this, gpuSceneDataBuffer]() { destroyBuffer(gpuSceneDataBuffer); });
    auto* sceneData = (GPUSceneData*)gpuSceneDataBuffer.info.pMappedData;
    *sceneData = this->sceneData;

    const auto sceneDescriptor =
        currentFrame.frameDescriptors.allocate(device, sceneDataDescriptorLayout);

    DescriptorWriter writer;
    writer.writeBuffer(
        0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeImage(
        1,
        csmShadowMap.imageView,
        defaultShadowMapSampler,
        VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(device, sceneDescriptor);

    return sceneDescriptor;
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

    for (auto& frameData : frames) {
        frameData.deletionQueue.flush();
    }

    meshCache.cleanup(*this);
    materialCache.cleanup(*this);

    deletionQueue.flush();

    destroyCommandBuffers();
    destroySyncStructures();

    descriptorAllocator.destroyPools(device);

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
}

void Renderer::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const
{
    VK_CHECK(vkResetFences(device, 1, &immFence));
    VK_CHECK(vkResetCommandBuffer(immCommandBuffer, 0));

    auto cmd = immCommandBuffer;
    const auto cmdBeginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    function(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    const auto cmdinfo = vkinit::commandBufferSubmitInfo(cmd);
    const auto submit = vkinit::submitInfo(&cmdinfo, nullptr, nullptr);

    VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, immFence));

    VK_CHECK(vkWaitForFences(device, 1, &immFence, true, NO_TIMEOUT));
}

void Renderer::beginDrawing(const GPUSceneData& sceneData)
{
    this->sceneData = sceneData;
    drawCommands.clear();
    getCurrentFrame().jointMatricesBuffer.clear();
}

void Renderer::addDrawCommand(MeshId id, const glm::mat4& transform)
{
    const auto& mesh = meshCache.getMesh(id);
    const auto worldBoundingSphere =
        edge::calculateBoundingSphereWorld(transform, mesh.boundingSphere, false);

    drawCommands.push_back(DrawCommand{
        .meshId = id,
        .transformMatrix = transform,
        .worldBoundingSphere = worldBoundingSphere,
    });
}

void Renderer::addDrawSkinnedMeshCommand(
    std::span<const MeshId> meshes,
    std::span<const SkinnedMesh> skinnedMeshes,
    const glm::mat4& transform,
    std::span<const glm::mat4> jointMatrices)
{
    auto& jointMatricesBuffer = getCurrentFrame().jointMatricesBuffer;
    const auto startIndex = jointMatricesBuffer.size;
    jointMatricesBuffer.append(jointMatrices);

    assert(meshes.size() == skinnedMeshes.size());
    for (std::size_t i = 0; i < meshes.size(); ++i) {
        const auto& mesh = meshCache.getMesh(meshes[i]);
        assert(mesh.hasSkeleton);

        // TODO: can calculate worldBounding sphere after skinning!
        const auto worldBoundingSphere =
            edge::calculateBoundingSphereWorld(transform, mesh.boundingSphere, true);
        drawCommands.push_back(DrawCommand{
            .meshId = meshes[i],
            .transformMatrix = transform,
            .worldBoundingSphere = worldBoundingSphere,
            .skinnedMesh = &skinnedMeshes[i],
            .jointMatricesStartIndex = (std::uint32_t)startIndex,
        });
    }
}

void Renderer::endDrawing()
{
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
            const auto& mesh1 = meshCache.getMesh(dc1.meshId);
            const auto& mesh2 = meshCache.getMesh(dc2.meshId);
            if (mesh1.materialId == mesh2.materialId) {
                return dc1.meshId < dc2.meshId;
            }
            return mesh1.materialId < mesh2.materialId;
        });
}

Scene Renderer::loadScene(const std::filesystem::path& path)
{
    util::LoadContext loadContext{
        .renderer = *this,
        .materialCache = materialCache,
        .meshCache = meshCache,
        .whiteTexture = whiteTexture,
    };
    util::SceneLoader loader;

    Scene scene;
    loader.loadScene(loadContext, scene, path);
    return scene;
}
