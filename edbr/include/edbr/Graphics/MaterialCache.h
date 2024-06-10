#pragma once

#include <vector>

#include <edbr/Graphics/IdTypes.h>
#include <edbr/Graphics/Material.h>

#include <edbr/Graphics/Vulkan/GPUBuffer.h>

class GfxDevice;

class MaterialCache {
    friend class ResourcesInspector;

public:
    void init(GfxDevice& gfxDevice);
    void cleanup(GfxDevice& gfxDevice);

    MaterialId addMaterial(GfxDevice& gfxDevice, Material material);
    const Material& getMaterial(MaterialId id) const;

    MaterialId getFreeMaterialId() const;
    MaterialId getPlaceholderMaterialId() const;

    const GPUBuffer& getMaterialDataBuffer() const { return materialDataBuffer; }
    VkDeviceAddress getMaterialDataBufferAddress() const { return materialDataBuffer.address; }

private:
    std::vector<Material> materials;

    static const auto MAX_MATERIALS = 1000;
    GPUBuffer materialDataBuffer;

    // material which is used for meshes without materials
    MaterialId placeholderMaterialId{NULL_MATERIAL_ID};

    ImageId defaultNormalMapTextureID{NULL_IMAGE_ID};
};
