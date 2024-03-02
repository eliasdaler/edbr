#pragma once

#include <cstdint>
#include <functional>

#include <vulkan/vulkan.h>

class VulkanImmediateExecutor {
public:
    void init(VkDevice device, std::uint32_t graphicsQueueFamily, VkQueue graphicsQueue);
    void cleanup(VkDevice device);

    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const;

private:
    bool initialized{false};

    VkDevice device;
    VkQueue graphicsQueue;

    VkCommandBuffer immCommandBuffer;
    VkCommandPool immCommandPool;
    VkFence immFence;
};
