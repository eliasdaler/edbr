#include "MeshCache.h"

#include <Renderer.h>

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

void MeshCache::cleanup(const Renderer& renderer)
{
    for (const auto& mesh : meshes) {
        renderer.destroyBuffer(mesh.buffers.indexBuffer);
        renderer.destroyBuffer(mesh.buffers.vertexBuffer);
        if (mesh.hasSkeleton) {
            renderer.destroyBuffer(mesh.skinnedVertexBuffer);
        }
    }
}
