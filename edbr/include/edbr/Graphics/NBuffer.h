#pragma once

#include <vector>

#include <edbr/Graphics/Vulkan/GPUBuffer.h>

class GfxDevice;

// NBuffer is a buffer with N (frame in flight) staging buffers and one buffer
// on GPU which gets read by shaders. It allows to easily upload new data to it
// each frame without any sync issues thanks to having N staging buffers.
class NBuffer {
public:
    void init(
        GfxDevice& gfxDevice,
        VkBufferUsageFlags usage,
        std::size_t dataSize,
        std::size_t numFramesInFlight,
        const char* label);

    void cleanup(GfxDevice& gfxDevice);

    void uploadNewData(
        VkCommandBuffer cmd,
        std::size_t frameIndex,
        void* newData,
        std::size_t dataSize,
        std::size_t offset = 0,
        bool sync = true) const;

    const GPUBuffer& getBuffer() const { return gpuBuffer; }

private:
    std::size_t framesInFlight{0};
    std::size_t gpuBufferSize{0};
    std::vector<GPUBuffer> stagingBuffers;
    GPUBuffer gpuBuffer;
    bool initialized{false};
};
