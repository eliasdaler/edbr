#include "BaseRenderer.h"

#include "GfxDevice.h"

#include <Graphics/CPUMesh.h>
#include <Graphics/Vulkan/Util.h>
#include <Math/Util.h>

BaseRenderer::BaseRenderer(GfxDevice& gfxDevice) : gfxDevice(gfxDevice)
{}

void BaseRenderer::init()
{
    executor = gfxDevice.createImmediateExecutor();
    initSamplers();
    initDefaultTextures();
    allocateMaterialDataBuffer();
    initDescriptors();
}

void BaseRenderer::cleanup()
{
    meshCache.cleanup(gfxDevice);
    imageCache.cleanup(gfxDevice);

    auto device = gfxDevice.getDevice();

    gfxDevice.destroyBuffer(materialDataBuffer);
    vkDestroyDescriptorSetLayout(device, meshMaterialLayout, nullptr);

    vkDestroySampler(device, defaultNearestSampler, nullptr);
    vkDestroySampler(device, defaultLinearSampler, nullptr);
    vkDestroySampler(device, defaultShadowMapSampler, nullptr);

    gfxDevice.destroyImage(whiteTexture);

    executor.cleanup(device);
}

void BaseRenderer::initSamplers()
{
    { // init nearest sampler
        const auto samplerCreateInfo = VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
        };
        VK_CHECK(vkCreateSampler(
            gfxDevice.getDevice(), &samplerCreateInfo, nullptr, &defaultNearestSampler));
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
        VK_CHECK(vkCreateSampler(
            gfxDevice.getDevice(), &samplerCreateInfo, nullptr, &defaultLinearSampler));
    }

    { // init shadow map sampler
        const auto samplerCreateInfo = VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .compareEnable = VK_TRUE,
            .compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL,
        };
        VK_CHECK(vkCreateSampler(
            gfxDevice.getDevice(), &samplerCreateInfo, nullptr, &defaultShadowMapSampler));
    }
}

void BaseRenderer::initDefaultTextures()
{
    { // create white texture
        whiteTexture = gfxDevice.createImage({
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .extent = VkExtent3D{1, 1, 1},
        });
        vkutil::addDebugLabel(gfxDevice.getDevice(), whiteTexture.image, "white texture");

        std::uint32_t white = 0xFFFFFFFF;
        gfxDevice.uploadImageData(whiteTexture, (void*)&white);
    }
}

void BaseRenderer::initDescriptors()
{
    const auto meshMaterialBindings = std::array<DescriptorLayoutBinding, 5>{{
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, // diffuse
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, // normal map
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, // metallic roughness
        {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, // emissive
    }};
    meshMaterialLayout = vkutil::buildDescriptorSetLayout(
        gfxDevice.getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, meshMaterialBindings);
}

void BaseRenderer::allocateMaterialDataBuffer()
{
    materialDataBuffer =
        gfxDevice
            .createBuffer(MAX_MATERIALS * sizeof(MaterialData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
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
    GPUMeshBuffers buffers;

    // create index buffer
    const auto indexBufferSize = cpuMesh.indices.size() * sizeof(std::uint32_t);
    buffers.indexBuffer = gfxDevice.createBuffer(
        indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    // create vertex buffer
    const auto vertexBufferSize = cpuMesh.vertices.size() * sizeof(CPUMesh::Vertex);
    buffers.vertexBuffer = gfxDevice.createBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    buffers.vertexBufferAddress = gfxDevice.getBufferAddress(buffers.vertexBuffer);

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
        vkCmdCopyBuffer(cmd, staging.buffer, buffers.vertexBuffer.buffer, 1, &vertexCopy);

        const auto indexCopy = VkBufferCopy{
            .srcOffset = vertexBufferSize,
            .dstOffset = 0,
            .size = indexBufferSize,
        };
        vkCmdCopyBuffer(cmd, staging.buffer, buffers.indexBuffer.buffer, 1, &indexCopy);
    });

    gfxDevice.destroyBuffer(staging);

    gpuMesh.buffers = buffers;

    if (gpuMesh.hasSkeleton) {
        // create skinning data buffer
        const auto skinningDataSize = cpuMesh.vertices.size() * sizeof(CPUMesh::SkinningData);
        gpuMesh.skinningDataBuffer = gfxDevice.createBuffer(
            skinningDataSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        gpuMesh.skinningDataBufferAddress = gfxDevice.getBufferAddress(gpuMesh.skinningDataBuffer);

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
    sm.skinnedVertexBufferAddress = gfxDevice.getBufferAddress(sm.skinnedVertexBuffer);
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
    };

    material.materialSet = gfxDevice.allocateDescriptorSet(meshMaterialLayout);

    struct ImageBinding {
        std::uint32_t binding;
        ImageId image;
        VkImageView placeholderImage;
    };
    const std::array<ImageBinding, 4> imageBindings{{
        {1, material.diffuseTexture, whiteTexture.imageView},
        {2, material.normalMapTexture, whiteTexture.imageView},
        {3, material.metallicRoughnessTexture, whiteTexture.imageView},
        {4, material.emissiveTexture, whiteTexture.imageView},
    }};

    DescriptorWriter writer;

    writer.writeBuffer(
        0,
        materialDataBuffer.buffer,
        sizeof(MaterialData),
        materialId * sizeof(MaterialData),
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (const auto& binding : imageBindings) {
        writer.writeImage(
            binding.binding,
            (binding.image == NULL_IMAGE_ID) ? binding.placeholderImage :
                                               getImage(binding.image).imageView,
            defaultLinearSampler,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
    writer.updateSet(gfxDevice.getDevice(), material.materialSet);

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

ImageId BaseRenderer::loadImageFromFile(
    const std::filesystem::path& path,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap)
{
    return imageCache.loadImageFromFile(gfxDevice, path, format, usage, mipMap);
}

const AllocatedImage& BaseRenderer::getImage(ImageId id) const
{
    return imageCache.getImage(id);
}
