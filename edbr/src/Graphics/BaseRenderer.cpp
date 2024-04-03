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
}

void BaseRenderer::cleanup()
{
    meshCache.cleanup(gfxDevice);

    auto device = gfxDevice.getDevice();
    executor.cleanup(device);
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

const GPUMesh& BaseRenderer::getMesh(MeshId id) const
{
    return meshCache.getMesh(id);
}
