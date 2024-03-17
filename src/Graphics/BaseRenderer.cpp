#include "BaseRenderer.h"

#include "GfxDevice.h"

#include <Graphics/CPUMesh.h>
#include <Graphics/Cubemap.h>
#include <Graphics/Vulkan/Util.h>
#include <Math/Util.h>

BaseRenderer::BaseRenderer(GfxDevice& gfxDevice) : gfxDevice(gfxDevice), imageCache(gfxDevice)
{}

void BaseRenderer::init()
{
    imageCache.bindlessSetManager.init(gfxDevice.getDevice());
    executor = gfxDevice.createImmediateExecutor();
    initSamplers();
    initDefaultTextures();
    allocateMaterialDataBuffer();
}

void BaseRenderer::cleanup()
{
    meshCache.cleanup(gfxDevice);
    imageCache.destroyImages();
    gfxDevice.destroyBuffer(materialDataBuffer);

    auto device = gfxDevice.getDevice();
    vkDestroySampler(device, nearestSampler, nullptr);
    vkDestroySampler(device, linearSampler, nullptr);
    vkDestroySampler(device, shadowMapSampler, nullptr);

    executor.cleanup(device);
    imageCache.bindlessSetManager.cleanup(device);
}

void BaseRenderer::initSamplers()
{
    // Keep in sync with bindless.glsl
    static const std::uint32_t nearestSamplerId = 0;
    static const std::uint32_t linearSamplerId = 1;
    static const std::uint32_t shadowSamplerId = 2;

    auto device = gfxDevice.getDevice();
    { // init nearest sampler
        const auto samplerCreateInfo = VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
        };
        VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &nearestSampler));
        vkutil::addDebugLabel(device, nearestSampler, "nearest");
        imageCache.bindlessSetManager.addSampler(device, nearestSamplerId, nearestSampler);
    }

    { // init linear sampler
        const auto samplerCreateInfo = VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            // TODO: make possible to disable anisotropy or set other values?
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = gfxDevice.getMaxAnisotropy(),
        };
        VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &linearSampler));
        vkutil::addDebugLabel(device, linearSampler, "linear");
        imageCache.bindlessSetManager.addSampler(device, linearSamplerId, linearSampler);
    }

    { // init shadow map sampler
        const auto samplerCreateInfo = VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .compareEnable = VK_TRUE,
            .compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL,
        };
        VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &shadowMapSampler));
        vkutil::addDebugLabel(device, shadowMapSampler, "shadow");
        imageCache.bindlessSetManager.addSampler(device, shadowSamplerId, shadowMapSampler);
    }
}

void BaseRenderer::initDefaultTextures()
{
    { // create white texture
        std::uint32_t pixel = 0xFFFFFFFF;
        whiteTextureID = createImage(
            {
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .extent = VkExtent3D{1, 1, 1},
            },
            "white texture",
            &pixel);
    }

    { // create default normal map texture
        std::uint32_t normal = 0xFFFF8080; // (0.5, 0.5, 1.0, 1.0)
        placeholderNormalMapTextureID = createImage(
            {
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .extent = VkExtent3D{1, 1, 1},
            },
            "normal map placeholder texture",
            &normal);
    }
}

void BaseRenderer::allocateMaterialDataBuffer()
{
    materialDataBuffer = gfxDevice.createBuffer(
        MAX_MATERIALS * sizeof(MaterialData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    vkutil::addDebugLabel(gfxDevice.getDevice(), materialDataBuffer.buffer, "material data");
}

MeshId BaseRenderer::addMesh(const CPUMesh& cpuMesh, MaterialId materialId)
{
    auto gpuMesh = GPUMesh{
        .numVertices = (std::uint32_t)cpuMesh.vertices.size(),
        .numIndices = (std::uint32_t)cpuMesh.indices.size(),
        .materialId = materialId,
        .minPos = cpuMesh.minPos,
        .maxPos = cpuMesh.maxPos,
        .hasSkeleton = cpuMesh.hasSkeleton,
    };

    std::vector<glm::vec3> positions(cpuMesh.vertices.size());
    for (std::size_t i = 0; i < cpuMesh.vertices.size(); ++i) {
        positions[i] = cpuMesh.vertices[i].position;
    }
    gpuMesh.boundingSphere = util::calculateBoundingSphere(positions);

    uploadMesh(cpuMesh, gpuMesh);

    return meshCache.addMesh(std::move(gpuMesh));
}

void BaseRenderer::uploadMesh(const CPUMesh& cpuMesh, GPUMesh& gpuMesh) const
{
    // create index buffer
    const auto indexBufferSize = cpuMesh.indices.size() * sizeof(std::uint32_t);
    gpuMesh.indexBuffer = gfxDevice.createBuffer(
        indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    // create vertex buffer
    const auto vertexBufferSize = cpuMesh.vertices.size() * sizeof(CPUMesh::Vertex);
    gpuMesh.vertexBuffer = gfxDevice.createBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

    const auto staging =
        gfxDevice
            .createBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    // copy data
    void* data = staging.info.pMappedData;
    memcpy(data, cpuMesh.vertices.data(), vertexBufferSize);
    memcpy((char*)data + vertexBufferSize, cpuMesh.indices.data(), indexBufferSize);

    executor.immediateSubmit([&](VkCommandBuffer cmd) {
        const auto vertexCopy = VkBufferCopy{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = vertexBufferSize,
        };
        vkCmdCopyBuffer(cmd, staging.buffer, gpuMesh.vertexBuffer.buffer, 1, &vertexCopy);

        const auto indexCopy = VkBufferCopy{
            .srcOffset = vertexBufferSize,
            .dstOffset = 0,
            .size = indexBufferSize,
        };
        vkCmdCopyBuffer(cmd, staging.buffer, gpuMesh.indexBuffer.buffer, 1, &indexCopy);
    });

    gfxDevice.destroyBuffer(staging);

    const auto vtxBufferName = cpuMesh.name + " (vtx)";
    const auto idxBufferName = cpuMesh.name + " (idx)";
    vkutil::
        addDebugLabel(gfxDevice.getDevice(), gpuMesh.vertexBuffer.buffer, vtxBufferName.c_str());
    vkutil::addDebugLabel(gfxDevice.getDevice(), gpuMesh.indexBuffer.buffer, idxBufferName.c_str());

    if (gpuMesh.hasSkeleton) {
        // create skinning data buffer
        const auto skinningDataSize = cpuMesh.vertices.size() * sizeof(CPUMesh::SkinningData);
        gpuMesh.skinningDataBuffer = gfxDevice.createBuffer(
            skinningDataSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

        const auto staging =
            gfxDevice.createBuffer(skinningDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        // copy data
        void* data = staging.info.pMappedData;
        memcpy(data, cpuMesh.skinningData.data(), vertexBufferSize);

        executor.immediateSubmit([&](VkCommandBuffer cmd) {
            const auto vertexCopy = VkBufferCopy{
                .srcOffset = 0,
                .dstOffset = 0,
                .size = skinningDataSize,
            };
            vkCmdCopyBuffer(cmd, staging.buffer, gpuMesh.skinningDataBuffer.buffer, 1, &vertexCopy);
        });

        gfxDevice.destroyBuffer(staging);
    }
}

SkinnedMesh BaseRenderer::createSkinnedMeshBuffer(MeshId meshId) const
{
    const auto& mesh = getMesh(meshId);
    SkinnedMesh sm;
    sm.skinnedVertexBuffer = gfxDevice.createBuffer(
        mesh.numVertices * sizeof(CPUMesh::Vertex),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    return sm;
}

MaterialId BaseRenderer::addMaterial(Material material)
{
    const auto materialId = materialCache.getFreeMaterialId();
    assert(materialId < MAX_MATERIALS - 1);

    MaterialData* data = (MaterialData*)materialDataBuffer.info.pMappedData;
    data[materialId] = MaterialData{
        .baseColor = material.baseColor,
        .metalRoughnessEmissive = glm::
            vec4{material.metallicFactor, material.roughnessFactor, material.emissiveFactor, 0.f},
        .diffuseTex =
            material.diffuseTexture != NULL_IMAGE_ID ? material.diffuseTexture : whiteTextureID,
        .normalTex = material.normalMapTexture != NULL_IMAGE_ID ? material.normalMapTexture :
                                                                  placeholderNormalMapTextureID,
        .metallicRoughnessTex = material.metallicRoughnessTexture != NULL_IMAGE_ID ?
                                    material.metallicRoughnessTexture :
                                    whiteTextureID,
        .emissiveTex =
            material.emissiveTexture != NULL_IMAGE_ID ? material.emissiveTexture : whiteTextureID,
    };

    materialCache.addMaterial(materialId, std::move(material));

    return materialId;
}

const Material& BaseRenderer::getMaterial(MaterialId id) const
{
    return materialCache.getMaterial(id);
}

const GPUMesh& BaseRenderer::getMesh(MeshId id) const
{
    return meshCache.getMesh(id);
}

ImageId BaseRenderer::createImage(
    const vkutil::CreateImageInfo& createInfo,
    const char* debugName,
    void* pixelData)
{
    auto image = gfxDevice.createImage(createInfo);
    if (debugName) {
        vkutil::addDebugLabel(gfxDevice.getDevice(), image.image, debugName);
    }
    if (pixelData) {
        gfxDevice.uploadImageData(image, pixelData);
    }
    return addImageToCache(std::move(image));
}

ImageId BaseRenderer::loadImageFromFile(
    const std::filesystem::path& path,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap)
{
    return imageCache.loadImageFromFile(path, format, usage, mipMap);
}

ImageId BaseRenderer::loadCubemap(const std::filesystem::path& dirPath)
{
    return addImageToCache(graphics::loadCubemap(gfxDevice, dirPath));
}

const AllocatedImage& BaseRenderer::getImage(ImageId id) const
{
    return imageCache.getImage(id);
}

ImageId BaseRenderer::addImageToCache(AllocatedImage img)
{
    return imageCache.addImage(std::move(img));
}

VkDescriptorSetLayout BaseRenderer::getBindlessDescSetLayout() const
{
    return imageCache.bindlessSetManager.getDescSetLayout();
}
VkDescriptorSet BaseRenderer::getBindlessDescSet() const
{
    return imageCache.bindlessSetManager.getDescSet();
}
