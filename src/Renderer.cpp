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

#include <imgui.h>
#include <imgui_impl_glfw.cpp>
#include <imgui_impl_vulkan.cpp>

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

    initMeshData();

    gradientConstants = ComputePushConstants{
        .data1 = glm::vec4{1, 0, 0, 1},
        .data2 = glm::vec4{0, 0, 1, 1},
    };
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

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Vulkan app", nullptr, nullptr);
    assert(window);

    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));

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
                    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
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
    drawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    drawImage.extent = drawImageExtent;

    VkImageUsageFlags usages{};
    usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    usages |= VK_IMAGE_USAGE_STORAGE_BIT;
    usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const auto imgCreateInfo = vkinit::imageCreateInfo(drawImage.format, usages, drawImageExtent);

    const auto imgAllocInfo = VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    vmaCreateImage(
        allocator, &imgCreateInfo, &imgAllocInfo, &drawImage.image, &drawImage.allocation, nullptr);

    const auto imageViewCreateInfo =
        vkinit::imageViewCreateInfo(drawImage.format, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &drawImage.imageView));

    deletionQueue.pushFunction([this]() {
        vkDestroyImageView(device, drawImage.imageView, nullptr);
        vmaDestroyImage(allocator, drawImage.image, drawImage.allocation);
    });
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

    VkShaderModule shader;
    if (!vkutil::loadShaderModule("shaders/gradient.comp.spv", device, &shader)) {
        std::cout << "Error while building compute shader" << std::endl;
        assert(false);
    }

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
    VkShaderModule vertexShader;
    assert(vkutil::loadShaderModule("shaders/colored_triangle.vert.spv", device, &vertexShader));
    VkShaderModule fragShader;
    assert(vkutil::loadShaderModule("shaders/colored_triangle.frag.spv", device, &fragShader));

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
    VkShaderModule vertexShader;
    assert(vkutil::loadShaderModule("shaders/mesh.vert.spv", device, &vertexShader));
    VkShaderModule fragShader;
    assert(vkutil::loadShaderModule("shaders/mesh.frag.spv", device, &fragShader));

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(GPUDrawPushConstants),
    };

    auto pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &bufferRange;

    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &meshPipelineLayout));

    meshPipeline = PipelineBuilder{meshPipelineLayout}
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

    ImGui_ImplGlfw_InitForVulkan(window, true);

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

void Renderer::initMeshData()
{
    std::array<Vertex, 4> rectVertices;

    rectVertices[0].position = {0.5, -0.5, 0};
    rectVertices[1].position = {0.5, 0.5, 0};
    rectVertices[2].position = {-0.5, -0.5, 0};
    rectVertices[3].position = {-0.5, 0.5, 0};

    rectVertices[0].color = {0, 0, 0, 1};
    rectVertices[1].color = {0.5, 0.5, 0.5, 1};
    rectVertices[2].color = {1, 0, 0, 1};
    rectVertices[3].color = {0, 1, 0, 1};

    std::array<std::uint32_t, 6> rectIndices{0, 1, 2, 2, 1, 3};

    rectangle = uploadMesh(rectIndices, rectVertices);
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
    VmaMemoryUsage memoryUsage)
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

void Renderer::destroyBuffer(const AllocatedBuffer& buffer)
{
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

GPUMeshBuffers Renderer::uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices)
{
    GPUMeshBuffers mesh;

    // create index buffer
    const auto indexBufferSize = indices.size() * sizeof(std::uint32_t);
    mesh.indexBuffer = createBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // create vertex buffer
    const auto vertexBufferSize = vertices.size() * sizeof(Vertex);
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

void Renderer::update(float dt)
{
    // ImGui::ShowDemoWindow();

    auto glmToArr = [](const glm::vec4& v) { return std::array<float, 4>{v.x, v.y, v.z, v.w}; };
    auto arrToGLM = [](const std::array<float, 4>& v) { return glm::vec4{v[0], v[1], v[2], v[3]}; };

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

void Renderer::run()
{
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // TODO: swapchain resize

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        float dt = 0.f; // TODO: compute
        update(dt);

        ImGui::Render();

        draw();
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
    const auto renderInfo = vkinit::renderingInfo(drawExtent, &colorAttachment, nullptr);
    vkCmdBeginRendering(cmd, &renderInfo);

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

    { // draw triangle
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }

    { // draw rect
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

        const auto pushConstants = GPUDrawPushConstants{
            .worldMatrix = glm::mat4{1.f},
            .vertexBuffer = rectangle.vertexBufferAddress,
        };
        vkCmdPushConstants(
            cmd,
            meshPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(GPUDrawPushConstants),
            &pushConstants);
        vkCmdBindIndexBuffer(cmd, rectangle.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
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

    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Renderer::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
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
