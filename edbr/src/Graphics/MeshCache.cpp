#include <edbr/Graphics/MeshCache.h>

#include <edbr/Graphics/GfxDevice.h>

MeshId MeshCache::addMesh(GPUMesh mesh)
{
    const auto id = meshes.size();
    meshes.push_back(std::move(mesh));
    return id;
}

const GPUMesh& MeshCache::getMesh(MeshId id) const
{
    return meshes.at(id);
}

void MeshCache::cleanup(const GfxDevice& gfxDevice)
{
    for (const auto& mesh : meshes) {
        gfxDevice.destroyBuffer(mesh.indexBuffer);
        gfxDevice.destroyBuffer(mesh.vertexBuffer);
        if (mesh.hasSkeleton) {
            gfxDevice.destroyBuffer(mesh.skinningDataBuffer);
        }
    }
}
