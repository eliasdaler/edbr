#include <edbr/Graphics/GameRenderer.h>

#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/FrustumCulling.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/MaterialCache.h>
#include <edbr/Graphics/MeshCache.h>
#include <edbr/Graphics/Scene.h>
#include <edbr/Graphics/Vulkan/Init.h>
#include <edbr/Graphics/Vulkan/Pipelines.h>
#include <edbr/Graphics/Vulkan/Util.h>

#include <imgui.h>

#include <numeric> // iota

#include <tracy/Tracy.hpp>

void GameRenderer::init(GfxDevice& gfxDevice, const glm::ivec2& drawImageSize)
{
    assert(!initialized && "GameRenderer::init already called");
    initialized = true;

    initSceneData(gfxDevice);

    samples = gfxDevice.getMaxSupportedSamplingCount(); // needs to be called before
                                                        // createDrawImage
    createDrawImage(gfxDevice, drawImageSize, true);

    skinningPipeline.init(gfxDevice);

    const auto cascadePercents = std::array{0.138f, 0.35f, 1.f};
    // const auto cascadePercents = std::array{0.04f, 0.1f, 1.f}; // good for far = 500.f
    csmPipeline.init(gfxDevice, cascadePercents);
    pointLightShadowMapPipeline.init(gfxDevice, pointLightMaxRange);

    meshPipeline.init(gfxDevice, drawImageFormat, depthImageFormat, samples);
    skyboxPipeline.init(gfxDevice, drawImageFormat, depthImageFormat, samples);

    depthResolvePipeline.init(gfxDevice, depthImageFormat);

    postFXPipeline.init(gfxDevice, drawImageFormat);
}

void GameRenderer::createDrawImage(
    GfxDevice& gfxDevice,
    const glm::ivec2& drawImageSize,
    bool firstCreate)
{
    const auto drawImageExtent = VkExtent3D{
        .width = (std::uint32_t)drawImageSize.x,
        .height = (std::uint32_t)drawImageSize.y,
        .depth = 1,
    };

    { // setup draw image
        VkImageUsageFlags usages{};
        usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

        auto createImageInfo = vkutil::CreateImageInfo{
            .format = drawImageFormat,
            .usage = usages,
            .extent = drawImageExtent,
            .samples = samples,
        };
        // reuse the same id if creating again
        drawImageId = gfxDevice.createImage(createImageInfo, "draw image", nullptr, drawImageId);

        if (firstCreate) {
            createImageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // no MSAA
            postFXDrawImageId = gfxDevice.createImage(createImageInfo, "post FX draw image");
        }
    }

    if (firstCreate) { // setup resolve image
        VkImageUsageFlags usages{};
        usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

        const auto createImageInfo = vkutil::CreateImageInfo{
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = usages,
            .extent = drawImageExtent,
        };
        resolveImageId = gfxDevice.createImage(createImageInfo, "resolve image");
    }

    { // setup depth image
        auto createInfo = vkutil::CreateImageInfo{
            .format = depthImageFormat,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .extent = drawImageExtent,
            .samples = samples,
        };

        // reuse the same id if creating again
        depthImageId = gfxDevice.createImage(createInfo, "depth image", nullptr, depthImageId);

        if (firstCreate) {
            createInfo.samples = VK_SAMPLE_COUNT_1_BIT; // NO MSAA
            resolveDepthImageId = gfxDevice.createImage(createInfo, "depth resolve");
        }
    }
}

void GameRenderer::initSceneData(GfxDevice& gfxDevice)
{
    sceneDataBuffer.init(
        gfxDevice,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        sizeof(GPUSceneData),
        graphics::FRAME_OVERLAP,
        "scene data");

    lightDataBuffer.init(
        gfxDevice,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        sizeof(GPULightData) * MAX_LIGHTS,
        graphics::FRAME_OVERLAP,
        "light data");
    lightDataGPU.resize(MAX_LIGHTS);
}

void GameRenderer::cleanup(GfxDevice& gfxDevice)
{
    assert(initialized && "GameRenderer::init was not called");

    const auto& device = gfxDevice.getDevice();

    lightDataBuffer.cleanup(gfxDevice);
    sceneDataBuffer.cleanup(gfxDevice);

    postFXPipeline.cleanup(device);
    depthResolvePipeline.cleanup(device);
    skyboxPipeline.cleanup(device);
    meshPipeline.cleanup(device);
    pointLightShadowMapPipeline.cleanup(gfxDevice);
    csmPipeline.cleanup(gfxDevice);
    skinningPipeline.cleanup(gfxDevice);
}

void GameRenderer::draw(
    VkCommandBuffer cmd,
    GfxDevice& gfxDevice,
    const MeshCache& meshCache,
    const MaterialCache& materialCache,
    const Camera& camera,
    const SceneData& sceneData)
{
    assert(initialized && "GameRenderer::init was not called");

    { // skinning
        { // Sync reading from skinning buffers with new writes
            const auto memoryBarrier = VkMemoryBarrier2{
                .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                .srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            };
            const auto dependencyInfo = VkDependencyInfo{
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .memoryBarrierCount = 1,
                .pMemoryBarriers = &memoryBarrier,
            };
            vkCmdPipelineBarrier2(cmd, &dependencyInfo);
        }

        vkutil::cmdBeginLabel(cmd, "Skinning");
        for (const auto& dc : meshDrawCommands) {
            if (!dc.skinnedMesh) {
                continue;
            }
            skinningPipeline.doSkinning(cmd, gfxDevice.getCurrentFrameIndex(), meshCache, dc);
        }
        vkutil::cmdEndLabel(cmd);

        { // Sync skinning with CSM
            const auto memoryBarrier = VkMemoryBarrier2{
                .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
            };
            const auto dependencyInfo = VkDependencyInfo{
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .memoryBarrierCount = 1,
                .pMemoryBarriers = &memoryBarrier,
            };
            vkCmdPipelineBarrier2(cmd, &dependencyInfo);
        }
    }

    if (sunlightIndex != -1) { // CSM
        ZoneScopedN("CSM");
        TracyVkZoneC(gfxDevice.getTracyVkCtx(), cmd, "CSM", tracy::Color::CornflowerBlue);
        vkutil::cmdBeginLabel(cmd, "CSM");

        auto& sunlight = lightDataGPU[sunlightIndex];
        csmPipeline.draw(
            cmd,
            gfxDevice,
            meshCache,
            camera,
            sunlight.direction,
            materialCache.getMaterialDataBuffer(),
            meshDrawCommands,
            shadowsEnabled);

        vkutil::cmdEndLabel(cmd);
    }

    std::vector<std::size_t> pointLightIndices;
    for (std::size_t i = 0; i < lightDataGPU.size(); ++i) {
        const auto& light = lightDataGPU[i];
        if (light.type == edbr::TYPE_POINT_LIGHT) {
            // TODO: check if this light should cast shadow or not
            pointLightIndices.push_back(i);
        }
    }
    if (!pointLightIndices.empty()) { // point light shadows
        ZoneScopedN("Point shadow");
        TracyVkZoneC(gfxDevice.getTracyVkCtx(), cmd, "Point shadow", tracy::Color::CornflowerBlue);
        vkutil::cmdBeginLabel(cmd, "Point shadow");

        pointLightShadowMapPipeline.beginFrame(cmd, gfxDevice, lightDataGPU, pointLightIndices);
        for (const auto lightIndex : pointLightIndices) {
            const auto& light = lightDataGPU[lightIndex];
            pointLightShadowMapPipeline.draw(
                cmd,
                gfxDevice,
                meshCache,
                (std::uint32_t)lightIndex,
                light.position,
                materialCache.getMaterialDataBuffer(),
                lightDataBuffer.getBuffer(),
                meshDrawCommands,
                shadowsEnabled);
        }
        pointLightShadowMapPipeline.endFrame(cmd, gfxDevice);
        vkutil::cmdEndLabel(cmd);
    }

    { // upload scene data - can only be done after shadow mapping was finished
        const auto gpuSceneData = GPUSceneData{
            .view = sceneData.camera.getView(),
            .proj = sceneData.camera.getProjection(),
            .viewProj = sceneData.camera.getViewProj(),
            .cameraPos = glm::vec4{sceneData.camera.getPosition(), 1.f},
            .ambientColor = LinearColorNoAlpha{sceneData.ambientColor},
            .ambientIntensity = sceneData.ambientIntensity,
            .fogColor = LinearColorNoAlpha{sceneData.fogColor},
            .fogDensity = sceneData.fogDensity,
            .cascadeFarPlaneZs =
                glm::vec4{
                    csmPipeline.cascadeFarPlaneZs[0],
                    csmPipeline.cascadeFarPlaneZs[1],
                    csmPipeline.cascadeFarPlaneZs[2],
                    0.f,
                },
            .csmLightSpaceTMs = csmPipeline.csmLightSpaceTMs,
            .csmShadowMapId = (std::uint32_t)csmPipeline.getShadowMap(),
            .pointLightFarPlane = pointLightMaxRange,
            .lightsBuffer = lightDataBuffer.getBuffer().address,
            .numLights = (std::uint32_t)lightDataGPU.size(),
            .sunlightIndex = sunlightIndex,
            .materialsBuffer = materialCache.getMaterialDataBufferAddress(),
        };
        sceneDataBuffer.uploadNewData(
            cmd, gfxDevice.getCurrentFrameIndex(), (void*)&gpuSceneData, sizeof(GPUSceneData));

        { // update lights
            // update point light shadow map IDs
            const auto& lightToSM = pointLightShadowMapPipeline.getLightToShadowMapId();
            for (std::size_t i = 0; i < lightDataGPU.size(); ++i) {
                if (auto it = lightToSM.find(i); it != lightToSM.end()) {
                    lightDataGPU[i].shadowMapID = it->second;
                }
            }

            lightDataBuffer.uploadNewData(
                cmd,
                gfxDevice.getCurrentFrameIndex(),
                (void*)lightDataGPU.data(),
                sizeof(GPULightData) * lightDataGPU.size());
        }
    }

    const auto& drawImage = gfxDevice.getImage(drawImageId);
    const auto& resolveImage = gfxDevice.getImage(resolveImageId);
    const auto& depthImage = gfxDevice.getImage(depthImageId);

    { // Geometry + Sky

        ZoneScopedN("Geometry");
        TracyVkZoneC(gfxDevice.getTracyVkCtx(), cmd, "Geometry", tracy::Color::ForestGreen);
        vkutil::cmdBeginLabel(cmd, "Geometry");

        vkutil::transitionImage(
            cmd,
            drawImage.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        vkutil::transitionImage(
            cmd,
            depthImage.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        if (isMultisamplingEnabled()) {
            vkutil::transitionImage(
                cmd,
                resolveImage.image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }

        const auto renderInfo = vkutil::createRenderingInfo({
            .renderExtent = drawImage.getExtent2D(),
            .colorImageView = drawImage.imageView,
            .colorImageClearValue = glm::vec4{0.f, 0.f, 0.f, 1.f},
            .depthImageView = depthImage.imageView,
            .depthImageClearValue = 0.f,
            .resolveImageView = isMultisamplingEnabled() ? resolveImage.imageView : VK_NULL_HANDLE,
        });

        vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

        meshPipeline.draw(
            cmd,
            drawImage.getExtent2D(),
            gfxDevice,
            meshCache,
            materialCache,
            camera,
            sceneDataBuffer.getBuffer(),
            meshDrawCommands,
            sortedMeshDrawCommands);

        // sky
        skyboxPipeline.draw(cmd, gfxDevice, camera);

        vkCmdEndRendering(cmd);
        vkutil::cmdEndLabel(cmd);
    }

    { // Sync Geometry/Sky with next pass
        const auto imageBarrier = VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image = isMultisamplingEnabled() ? resolveImage.image : drawImage.image,
            .subresourceRange = vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT),
        };
        const auto depthBarrier = VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image = depthImage.image,
            .subresourceRange = vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT),
        };
        const auto barriers = std::array{imageBarrier, depthBarrier};
        const auto dependencyInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = barriers.size(),
            .pImageMemoryBarriers = barriers.data(),
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }

    if (isMultisamplingEnabled()) {
        ZoneScopedN("Depth resolve");
        TracyVkZoneC(gfxDevice.getTracyVkCtx(), cmd, "Depth resolve", tracy::Color::ForestGreen);
        vkutil::cmdBeginLabel(cmd, "Depth resolve");

        const auto& resolveDepthImage = gfxDevice.getImage(resolveDepthImageId);
        vkutil::transitionImage(
            cmd,
            resolveDepthImage.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        const auto renderInfo = vkutil::createRenderingInfo({
            .renderExtent = resolveDepthImage.getExtent2D(),
            .depthImageView = resolveDepthImage.imageView,
        });

        vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

        depthResolvePipeline.draw(cmd, gfxDevice, depthImage, vkutil::sampleCountToInt(samples));

        vkCmdEndRendering(cmd);

        // sync with post fx
        vkutil::transitionImage(
            cmd,
            resolveDepthImage.image,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkutil::cmdEndLabel(cmd);
    }

    { // post FX
        ZoneScopedN("Post FX");
        TracyVkZoneC(gfxDevice.getTracyVkCtx(), cmd, "Post FX", tracy::Color::Purple);
        vkutil::cmdBeginLabel(cmd, "Post FX");

        const auto& postFXDrawImage = gfxDevice.getImage(postFXDrawImageId);
        vkutil::transitionImage(
            cmd,
            postFXDrawImage.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        const auto renderInfo = vkutil::createRenderingInfo({
            .renderExtent = postFXDrawImage.getExtent2D(),
            .colorImageView = postFXDrawImage.imageView,
        });

        vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);
        if (isMultisamplingEnabled()) {
            const auto& resolveDepthImage = gfxDevice.getImage(resolveDepthImageId);
            postFXPipeline
                .draw(cmd, gfxDevice, resolveImage, resolveDepthImage, sceneDataBuffer.getBuffer());
        } else {
            postFXPipeline.draw(cmd, gfxDevice, drawImage, depthImage, sceneDataBuffer.getBuffer());
        }
        vkCmdEndRendering(cmd);

        vkutil::cmdEndLabel(cmd);
    }
}

void GameRenderer::updateDevTools(GfxDevice& gfxDevice, float dt)
{
    ImGui::DragFloat3("Cascades", csmPipeline.percents.data(), 0.1f, 0.f, 1.f);

    ImGui::Checkbox("Shadows", &shadowsEnabled);

    if (ImGui::BeginCombo("MSAA", vkutil::sampleCountToString(samples))) {
        static const auto counts = std::array{
            VK_SAMPLE_COUNT_1_BIT,
            VK_SAMPLE_COUNT_2_BIT,
            VK_SAMPLE_COUNT_4_BIT,
            VK_SAMPLE_COUNT_8_BIT,
            VK_SAMPLE_COUNT_16_BIT,
            VK_SAMPLE_COUNT_32_BIT,
            VK_SAMPLE_COUNT_64_BIT,
        };
        for (const auto& count : counts) {
            if (!gfxDevice.deviceSupportsSamplingCount(count)) {
                continue;
            }
            bool isSelected = (count == samples);
            if (ImGui::Selectable(vkutil::sampleCountToString(count), isSelected)) {
                samples = count;
                onMultisamplingStateUpdate(gfxDevice);
            }
        }
        ImGui::EndCombo();
    }
}

bool GameRenderer::isMultisamplingEnabled() const
{
    return samples != VK_SAMPLE_COUNT_1_BIT;
}

void GameRenderer::onMultisamplingStateUpdate(GfxDevice& gfxDevice)
{
    gfxDevice.waitIdle();

    { // cleanup old state
        meshPipeline.cleanup(gfxDevice.getDevice());
        skyboxPipeline.cleanup(gfxDevice.getDevice());

        const auto& drawImage = gfxDevice.getImage(drawImageId);
        const auto& depthImage = gfxDevice.getImage(depthImageId);

        gfxDevice.destroyImage(depthImage);
        gfxDevice.destroyImage(drawImage);
    }

    const auto& drawImage = gfxDevice.getImage(drawImageId);
    const auto prevDrawImageSize =
        glm::ivec2{drawImage.getExtent2D().width, drawImage.getExtent2D().height};
    createDrawImage(gfxDevice, prevDrawImageSize, false);

    // recreate pipelines
    meshPipeline.init(gfxDevice, drawImageFormat, depthImageFormat, samples);
    skyboxPipeline.init(gfxDevice, drawImageFormat, depthImageFormat, samples);
}

void GameRenderer::setSkyboxImage(ImageId skyboxImageId)
{
    skyboxPipeline.setSkyboxImage(skyboxImageId);
}

void GameRenderer::beginDrawing(GfxDevice& gfxDevice)
{
    meshDrawCommands.clear();
    lightDataGPU.clear();
    sunlightIndex = -1;
    skinningPipeline.beginDrawing(gfxDevice.getCurrentFrameIndex());
}

void GameRenderer::endDrawing()
{
    sortDrawList();
}

void GameRenderer::addLight(const Light& light, const Transform& transform)
{
    if (light.type == LightType::Directional) {
        assert(sunlightIndex == -1 && "directional light was already added before in the frame");
        sunlightIndex = (std::uint32_t)lightDataGPU.size();
    }

    GPULightData ld{};

    ld.position = transform.getPosition();
    ld.type = light.getGPUType();
    ld.direction = transform.getLocalFront();
    ld.range = light.range;
    if (light.range == 0) {
        if (light.type == LightType::Point) {
            ld.range = pointLightMaxRange;
        } else if (light.type == LightType::Spot) {
            ld.range = spotLightMaxRange;
        }
    }

    ld.color = LinearColorNoAlpha{light.color};
    ld.intensity = light.intensity;

    ld.scaleOffset.x = light.scaleOffset.x;
    ld.scaleOffset.y = light.scaleOffset.y;

    ld.shadowMapID = 0;

    lightDataGPU.push_back(ld);
}

void GameRenderer::drawMesh(
    const MeshCache& meshCache,
    MeshId id,
    const glm::mat4& transform,
    MaterialId materialId,
    bool castShadow)
{
    const auto& mesh = meshCache.getMesh(id);
    const auto worldBoundingSphere =
        edge::calculateBoundingSphereWorld(transform, mesh.boundingSphere, false);
    assert(materialId != NULL_MATERIAL_ID);

    meshDrawCommands.push_back(MeshDrawCommand{
        .meshId = id,
        .transformMatrix = transform,
        .worldBoundingSphere = worldBoundingSphere,
        .materialId = materialId,
        .castShadow = castShadow,
    });
}

std::size_t GameRenderer::appendJointMatrices(
    GfxDevice& gfxDevice,
    std::span<const glm::mat4> jointMatrices)
{
    return skinningPipeline.appendJointMatrices(jointMatrices, gfxDevice.getCurrentFrameIndex());
}

void GameRenderer::drawSkinnedMesh(
    const MeshCache& meshCache,
    MeshId meshId,
    const glm::mat4& transform,
    MaterialId materialId,
    const SkinnedMesh& skinnedMesh,
    std::size_t jointMatricesStartIndex)
{
    const auto& mesh = meshCache.getMesh(meshId);
    assert(mesh.hasSkeleton);
    assert(materialId != NULL_MATERIAL_ID);

    const auto worldBoundingSphere =
        edge::calculateBoundingSphereWorld(transform, mesh.boundingSphere, true);
    meshDrawCommands.push_back(MeshDrawCommand{
        .meshId = meshId,
        .transformMatrix = transform,
        .worldBoundingSphere = worldBoundingSphere,
        .materialId = materialId,
        .skinnedMesh = &skinnedMesh,
        .jointMatricesStartIndex = (std::uint32_t)jointMatricesStartIndex,
    });
}

void GameRenderer::sortDrawList()
{
    sortedMeshDrawCommands.clear();
    sortedMeshDrawCommands.resize(meshDrawCommands.size());
    std::iota(sortedMeshDrawCommands.begin(), sortedMeshDrawCommands.end(), 0);

    std::sort(
        sortedMeshDrawCommands.begin(),
        sortedMeshDrawCommands.end(),
        [this](const auto& i1, const auto& i2) {
            const auto& dc1 = meshDrawCommands[i1];
            const auto& dc2 = meshDrawCommands[i2];
            return dc1.meshId < dc2.meshId;
        });
}

const GPUImage& GameRenderer::getDrawImage(GfxDevice& gfxDevice) const
{
    // this is our "real" draw image as far as other systems are concerned
    return gfxDevice.getImage(postFXDrawImageId);
}

VkFormat GameRenderer::getDrawImageFormat() const
{
    return drawImageFormat;
}

const GPUImage& GameRenderer::getDepthImage(GfxDevice& gfxDevice) const
{
    return gfxDevice.getImage(isMultisamplingEnabled() ? resolveDepthImageId : depthImageId);
}

VkFormat GameRenderer::getDepthImageFormat() const
{
    return depthImageFormat;
}
