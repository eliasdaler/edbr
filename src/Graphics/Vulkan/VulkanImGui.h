#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>

struct VulkanImGuiData {
    VkDescriptorPool imguiPool;
    VkFormat swapchainViewFormat;
    std::vector<VkImageView> swapchainViews;
};

struct SDL_Window;

namespace vkutil
{
void initSwapchainViews(
    VulkanImGuiData& imguiData,
    VkDevice device,
    const std::vector<VkImage> swapchainImages);
void initImGui(
    VulkanImGuiData& imguiData,
    VkInstance instance,
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    std::uint32_t graphicsQueueFamily,
    VkQueue graphicsQueue,
    SDL_Window* window);
void cleanupImGui(VulkanImGuiData& imguiData, VkDevice device);
void drawImGui(
    VulkanImGuiData& imguiData,
    VkCommandBuffer cmd,
    VkExtent2D swapchainExtent,
    std::uint32_t swapchainImageIndex);
}
