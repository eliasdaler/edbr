#include <edbr/Graphics/NBuffer.h>

#include <edbr/Graphics/GfxDevice.h>

#include <cassert>
#include <cstring>

#include <volk.h>

#include <edbr/Graphics/Vulkan/Util.h>

void NBuffer::init(
    GfxDevice& gfxDevice,
    VkBufferUsageFlags usage,
    std::size_t dataSize,
    std::size_t numFramesInFlight,
    const char* label)
{
    assert(numFramesInFlight > 0);
    assert(dataSize > 0);

    framesInFlight = numFramesInFlight;
    gpuBufferSize = dataSize;

    gpuBuffer = gfxDevice.createBuffer(
        dataSize, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    vkutil::addDebugLabel(gfxDevice.getDevice(), gpuBuffer.buffer, label);

    for (std::size_t i = 0; i < numFramesInFlight; ++i) {
        stagingBuffers.push_back(gfxDevice.createBuffer(
            dataSize, usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST));
    }

    initialized = true;
}

void NBuffer::cleanup(GfxDevice& device)
{
    for (const auto& stagingBuffer : stagingBuffers) {
        device.destroyBuffer(stagingBuffer);
    }
    stagingBuffers.clear();

    device.destroyBuffer(gpuBuffer);

    initialized = false;
}

void NBuffer::uploadNewData(
    VkCommandBuffer cmd,
    std::size_t frameIndex,
    void* newData,
    std::size_t dataSize,
    std::size_t offset,
    bool sync) const
{
    assert(initialized);
    assert(frameIndex < framesInFlight);
    assert(offset + dataSize <= gpuBufferSize && "NBuffer::uploadNewData: out of bounds write");

    if (dataSize == 0) {
        return;
    }

    // sync with previous read
    if (sync) {
        const auto bufferBarrier = VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .buffer = gpuBuffer.buffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        const auto dependencyInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers = &bufferBarrier,
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }

    auto& staging = stagingBuffers[frameIndex];
    auto* mappedData = reinterpret_cast<std::uint8_t*>(staging.info.pMappedData);
    memcpy((void*)&mappedData[offset], newData, dataSize);

    const auto region = VkBufferCopy2{
        .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
        .srcOffset = (VkDeviceSize)offset,
        .dstOffset = (VkDeviceSize)offset,
        .size = dataSize,
    };
    const auto bufCopyInfo = VkCopyBufferInfo2{
        .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
        .srcBuffer = staging.buffer,
        .dstBuffer = gpuBuffer.buffer,
        .regionCount = 1,
        .pRegions = &region,
    };

    vkCmdCopyBuffer2(cmd, &bufCopyInfo);

    if (sync) { // sync write
        const auto bufferBarrier = VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
            .buffer = gpuBuffer.buffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        const auto dependencyInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers = &bufferBarrier,
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }
}
