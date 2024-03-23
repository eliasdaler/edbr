#include <edbr/Graphics/Vulkan/VulkanImmediateExecutor.h>

#include <volk.h>

#include <edbr/Graphics/Vulkan/Init.h>
#include <edbr/Graphics/Vulkan/Util.h>

namespace
{
static constexpr auto NO_TIMEOUT = std::numeric_limits<std::uint64_t>::max();
}

void VulkanImmediateExecutor::init(
    VkDevice device,
    std::uint32_t graphicsQueueFamily,
    VkQueue graphicsQueue)
{
    assert(!initialized);

    this->device = device;
    this->graphicsQueue = graphicsQueue;

    const auto poolCreateInfo = vkinit::
        commandPoolCreateInfo(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphicsQueueFamily);
    VK_CHECK(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &immCommandPool));

    const auto cmdAllocInfo = vkinit::commandBufferAllocateInfo(immCommandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &immCommandBuffer));

    const auto fenceCreateInfo = VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &immFence));

    initialized = true;
}

void VulkanImmediateExecutor::cleanup(VkDevice device)
{
    assert(initialized);
    vkDestroyCommandPool(device, immCommandPool, nullptr);
    vkDestroyFence(device, immFence, nullptr);
}

void VulkanImmediateExecutor::immediateSubmit(
    std::function<void(VkCommandBuffer cmd)>&& function) const
{
    assert(initialized);
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
