#pragma once

#include <filesystem>

#include <webgpu/webgpu_cpp.h>

#include <glm/vec4.hpp>

#include <Graphics/Texture.h>

struct ImageData;

class MipMapGenerator;

namespace util
{
void initWebGPU();
wgpu::Adapter requestAdapter(
    const wgpu::Instance& instance,
    wgpu::RequestAdapterOptions const* options);
wgpu::Device requestDevice(const wgpu::Adapter& adapter, wgpu::DeviceDescriptor const* descriptor);

void defaultShaderCompilationCallback(
    WGPUCompilationInfoRequestStatus status,
    WGPUCompilationInfo const* compilationInfo,
    void* userdata);

// WebGPU's writeBuffer only allows to have writeBuffer's contentSize to be a
// multiple of 4 bytes. This means that the number of triangles in index buffer
// has to be even when its type is uint16_t (6*2=12 bytes).
// Otherwise, with uneven number of tris, you get (num tris * 3 * 2) = N*4+2 bytes size buffer.
void padBufferToFourBytes(std::vector<std::uint16_t>& indices);

std::uint32_t calculateMipCount(int imageWidth, int imageHeight);

struct TextureLoadContext {
    const wgpu::Device& device;
    const wgpu::Queue& queue;
    MipMapGenerator& mipMapGenerator;
};

Texture loadTexture(
    const TextureLoadContext& ctx,
    const std::filesystem::path& path,
    wgpu::TextureFormat format,
    bool generateMips = true);

Texture loadTexture(
    const TextureLoadContext& ctx,
    wgpu::TextureFormat format,
    const ImageData& data,
    bool generateMips = true,
    const char* label = nullptr);

// creates a 1x1 px texture of specified color
Texture createPixelTexture(
    const TextureLoadContext& ctx,
    wgpu::TextureFormat format,
    const glm::vec4& color,
    const char* label = nullptr);

Texture loadCubemap(
    const TextureLoadContext& ctx,
    const std::filesystem::path& imagesDir,
    bool generateMips = true,
    const char* label = nullptr);

} // namespace util
