#pragma once

#include "Types.h"

#include <vector>

// NBuffer is a buffer with N (frame in flight) staging buffers and one buffer
// on GPU which gets read by shaders. It allows to easily upload new data to it
// each frame without any sync issues thanks to having N staging buffers.
class NBuffer {
public:
    void init(
        VkDevice device,
        VmaAllocator allocator,
        VkBufferUsageFlags usage,
        std::size_t dataSize,
        std::size_t numFramesInFlight,
        const char* label);

    void cleanup(VkDevice device, VmaAllocator allocator);

    void uploadNewData(
        VkCommandBuffer cmd,
        std::size_t frameIndex,
        void* newData,
        std::size_t dataSize);

    const AllocatedBuffer& getBuffer() { return gpuBuffer; }

private:
    std::size_t framesInFlight{0};
    std::size_t gpuBufferSize{0};
    std::vector<AllocatedBuffer> stagingBuffers;
    AllocatedBuffer gpuBuffer;
    bool initialized{false};
};
