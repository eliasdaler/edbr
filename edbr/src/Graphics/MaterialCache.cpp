#include <edbr/Graphics/MaterialCache.h>

#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Vulkan/Util.h>

void MaterialCache::init(GfxDevice& gfxDevice)
{
    assert(!initialized && "MaterialCache::init was already called");
    initialized = true;

    materialDataBuffer = gfxDevice.createBuffer(
        MAX_MATERIALS * sizeof(MaterialData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    vkutil::addDebugLabel(gfxDevice.getDevice(), materialDataBuffer.buffer, "material data");

    { // create default normal map texture
        std::uint32_t normal = 0xFFFF8080; // (0.5, 0.5, 1.0, 1.0)
        defaultNormalMapTextureID = gfxDevice.createImage(
            {
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .extent = VkExtent3D{1, 1, 1},
            },
            "normal map placeholder texture",
            &normal);
    }

    Material placeholderMaterial{.name = "PLACEHOLDER_MATERIAL"};
    placeholderMaterialId = addMaterial(gfxDevice, placeholderMaterial);
}

void MaterialCache::cleanup(GfxDevice& gfxDevice)
{
    assert(initialized && "MaterialCache::init not called");
    gfxDevice.destroyBuffer(materialDataBuffer);
}

MaterialId MaterialCache::addMaterial(GfxDevice& gfxDevice, Material material)
{
    assert(initialized && "MaterialCache::init not called");

    const auto getTextureOrElse = [](ImageId imageId, ImageId placeholder) {
        return imageId != NULL_IMAGE_ID ? imageId : placeholder;
    };

    // store on GPU
    MaterialData* data = (MaterialData*)materialDataBuffer.info.pMappedData;
    auto whiteTextureID = gfxDevice.getWhiteTextureID();
    auto id = getFreeMaterialId();
    assert(id < MAX_MATERIALS);
    data[id] = MaterialData{
        .baseColor = material.baseColor,
        .metalRoughnessEmissive = glm::
            vec4{material.metallicFactor, material.roughnessFactor, material.emissiveFactor, 0.f},
        .diffuseTex = getTextureOrElse(material.diffuseTexture, whiteTextureID),
        .normalTex = getTextureOrElse(material.normalMapTexture, defaultNormalMapTextureID),
        .metallicRoughnessTex = getTextureOrElse(material.metallicRoughnessTexture, whiteTextureID),
        .emissiveTex = getTextureOrElse(material.emissiveTexture, whiteTextureID),
    };

    // store on CPU
    materials.push_back(std::move(material));

    return id;
}

const Material& MaterialCache::getMaterial(MaterialId id) const
{
    assert(initialized && "MaterialCache::init not called");
    return materials.at(id);
}

MaterialId MaterialCache::getFreeMaterialId() const
{
    assert(initialized && "MaterialCache::init not called");
    return materials.size();
}

MaterialId MaterialCache::getPlaceholderMaterialId() const
{
    assert(initialized);
    assert(placeholderMaterialId != NULL_MATERIAL_ID && "MaterialCache::init not called");
    return placeholderMaterialId;
}
