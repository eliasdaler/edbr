#include "VulkanImGui.h"

#include <iterator>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include <volk.h>

#include "Init.h"
#include "Util.h"

namespace vkutil
{
void initSwapchainViews(
    VulkanImGuiData& imguiData,
    VkDevice device,
    const std::vector<VkImage> swapchainImages)
{
    imguiData.swapchainViewFormat = VK_FORMAT_B8G8R8A8_UNORM;
    imguiData.swapchainViews.resize(swapchainImages.size());
    for (std::size_t i = 0; i < swapchainImages.size(); ++i) {
        const auto imageViewCreateInfo = vkinit::imageViewCreateInfo(
            imguiData.swapchainViewFormat, swapchainImages[i], VK_IMAGE_ASPECT_COLOR_BIT);
        VK_CHECK(
            vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imguiData.swapchainViews[i]));
    }
}

void initImGui(
    VulkanImGuiData& imguiData,
    VkInstance instance,
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    std::uint32_t graphicsQueueFamily,
    VkQueue graphicsQueue,
    SDL_Window* window)
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

    VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiData.imguiPool));

    ImGui::CreateContext();

    auto initInfo = ImGui_ImplVulkan_InitInfo{
        .Instance = instance,
        .PhysicalDevice = physicalDevice,
        .Device = device,
        .QueueFamily = graphicsQueueFamily,
        .Queue = graphicsQueue,
        .DescriptorPool = imguiData.imguiPool,
        .MinImageCount = 3,
        .ImageCount = 3,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo =
            VkPipelineRenderingCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &imguiData.swapchainViewFormat,
            },
        .CheckVkResultFn = [](VkResult res) { assert(res == VK_SUCCESS); },
    };

    ImGui_ImplVulkan_LoadFunctions(
        [](const char* function_name, void* instance) {
            return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance*>(instance)), function_name);
        },
        &instance);
    ImGui_ImplVulkan_Init(&initInfo);
    ImGui_ImplSDL2_InitForVulkan(window);
}

void cleanupImGui(VulkanImGuiData& imguiData, VkDevice device)
{
    for (auto imageView : imguiData.swapchainViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    imguiData.swapchainViews.clear();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(device, imguiData.imguiPool, nullptr);
}

void drawImGui(
    VulkanImGuiData& imguiData,
    VkCommandBuffer cmd,
    VkExtent2D swapchainExtent,
    std::uint32_t swapchainImageIndex)
{
    const auto colorAttachment = vkinit::
        attachmentInfo(imguiData.swapchainViews[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL);
    const auto renderInfo = vkinit::renderingInfo(swapchainExtent, &colorAttachment, nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}
}
