#include <edbr/Graphics/BaseRenderer.h>

#include <edbr/Graphics/GfxDevice.h>

#include <edbr/Graphics/CPUMesh.h>
#include <edbr/Graphics/Cubemap.h>
#include <edbr/Graphics/Vulkan/Util.h>
#include <edbr/Math/Util.h>

BaseRenderer::BaseRenderer(GfxDevice& gfxDevice) : gfxDevice(gfxDevice)
{}

void BaseRenderer::init()
{
    executor = gfxDevice.createImmediateExecutor();
    initSamplers();
    initDefaultTextures();
    allocateMaterialDataBuffer();
}

void BaseRenderer::cleanup()
{
    meshCache.cleanup(gfxDevice);
    gfxDevice.destroyBuffer(materialDataBuffer);

    auto device = gfxDevice.getDevice();
    vkDestroySampler(device, nearestSampler, nullptr);
    vkDestroySampler(device, linearSampler, nullptr);
    vkDestroySampler(device, shadowMapSampler, nullptr);

    executor.cleanup(device);
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
        gfxDevice.getBindlessSetManager().addSampler(device, nearestSamplerId, nearestSampler);
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
        gfxDevice.getBindlessSetManager().addSampler(device, linearSamplerId, linearSampler);
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
        gfxDevice.getBindlessSetManager().addSampler(device, shadowSamplerId, shadowMapSampler);
    }
}

void BaseRenderer::initDefaultTextures()
{
    { // create default normal map texture
        std::uint32_t normal = 0xFFFF8080; // (0.5, 0.5, 1.0, 1.0)
        placeholderNormalMapTextureID = gfxDevice.createImage(
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
        .diffuseTex = material.diffuseTexture != NULL_IMAGE_ID ? material.diffuseTexture :
                                                                 gfxDevice.getWhiteTextureID(),
        .normalTex = material.normalMapTexture != NULL_IMAGE_ID ? material.normalMapTexture :
                                                                  placeholderNormalMapTextureID,
        .metallicRoughnessTex = material.metallicRoughnessTexture != NULL_IMAGE_ID ?
                                    material.metallicRoughnessTexture :
                                    gfxDevice.getWhiteTextureID(),
        .emissiveTex = material.emissiveTexture != NULL_IMAGE_ID ? material.emissiveTexture :
                                                                   gfxDevice.getWhiteTextureID(),
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
