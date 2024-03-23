#include <edbr/Graphics/GameRenderer.h>

#include <edbr/Graphics/BaseRenderer.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/FrustumCulling.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Scene.h>
#include <edbr/Graphics/Vulkan/Init.h>
#include <edbr/Graphics/Vulkan/Pipelines.h>

#include <imgui.h>

#include <numeric> // iota

#include <tracy/Tracy.hpp>

GameRenderer::GameRenderer(GfxDevice& gfxDevice, BaseRenderer& baseRenderer) :
    gfxDevice(gfxDevice), baseRenderer(baseRenderer)
{}

void GameRenderer::init()
{
    initSceneData();

    samples = gfxDevice.getMaxSupportedSamplingCount(); // needs to be called before
                                                        // createDrawImage
    createDrawImage(gfxDevice.getSwapchainExtent(), true);

    skinningPipeline.init(gfxDevice);

    const auto cascadePercents = std::array{0.06f, 0.2f, 0.5f};
    // const auto cascadePercents =  std::array{0.22f, 0.51f, 1.f};
    csmPipeline.init(gfxDevice, cascadePercents);

    meshPipeline.init(gfxDevice, drawImageFormat, depthImageFormat, samples);
    skyboxPipeline.init(gfxDevice, drawImageFormat, depthImageFormat, samples);

    depthResolvePipeline.init(gfxDevice, depthImageFormat);

    postFXPipeline.init(gfxDevice, drawImageFormat);
}

void GameRenderer::createDrawImage(VkExtent2D extent, bool firstCreate)
{
    const auto drawImageExtent = VkExtent3D{
        .width = extent.width,
        .height = extent.height,
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
            postFXDrawImage = gfxDevice.createImageRaw(createImageInfo);
            vkutil::
                addDebugLabel(gfxDevice.getDevice(), postFXDrawImage.image, "post FX draw image");
            vkutil::addDebugLabel(
                gfxDevice.getDevice(), postFXDrawImage.imageView, "post FX draw image view");
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
        depthImageId =
            gfxDevice.createImage(createInfo, "draw image (depth)", nullptr, depthImageId);

        if (firstCreate) {
            createInfo.samples = VK_SAMPLE_COUNT_1_BIT; // NO MSAA
            resolveDepthImage = gfxDevice.createImageRaw(createInfo);

            vkutil::addDebugLabel(
                gfxDevice.getDevice(), resolveDepthImage.image, "draw image (depth resolve)");
            vkutil::addDebugLabel(
                gfxDevice.getDevice(),
                resolveDepthImage.imageView,
                "draw image depth resolve view");
        }
    }
}

void GameRenderer::initSceneData()
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
    lightDataCPU.resize(MAX_LIGHTS);
}

void GameRenderer::draw(VkCommandBuffer cmd, const Camera& camera, const SceneData& sceneData)
{
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
            skinningPipeline.doSkinning(cmd, gfxDevice.getCurrentFrameIndex(), baseRenderer, dc);
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

        auto& sunlight = lightDataCPU[sunlightIndex];
        csmPipeline.draw(
            cmd,
            gfxDevice,
            baseRenderer,
            camera,
            sunlight.direction,
            meshDrawCommands,
            shadowsEnabled);

        vkutil::cmdEndLabel(cmd);
    }

    { // upload scene data - can only be done after CSM has finished
        const auto gpuSceneData = GPUSceneData{
            .view = sceneData.camera.getView(),
            .proj = sceneData.camera.getProjection(),
            .viewProj = sceneData.camera.getViewProj(),
            .cameraPos = glm::vec4{sceneData.camera.getPosition(), 1.f},
            .ambientColor = glm::vec3{sceneData.ambientColor},
            .ambientIntensity = sceneData.ambientIntensity,
            .fogColor = glm::vec3{sceneData.fogColor},
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
            .lightsBuffer = lightDataBuffer.getBuffer().address,
            .numLights = (std::uint32_t)lightDataCPU.size(),
            .sunlightIndex = sunlightIndex,
            .materialsBuffer = baseRenderer.materialDataBuffer.address,
        };
        sceneDataBuffer.uploadNewData(
            cmd, gfxDevice.getCurrentFrameIndex(), (void*)&gpuSceneData, sizeof(GPUSceneData));
        lightDataBuffer.uploadNewData(
            cmd,
            gfxDevice.getCurrentFrameIndex(),
            (void*)lightDataCPU.data(),
            sizeof(GPULightData) * lightDataCPU.size());
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
            baseRenderer,
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
            postFXPipeline
                .draw(cmd, gfxDevice, resolveImage, depthImage, sceneDataBuffer.getBuffer());
        } else {
            postFXPipeline.draw(cmd, gfxDevice, drawImage, depthImage, sceneDataBuffer.getBuffer());
        }
        vkCmdEndRendering(cmd);

        vkutil::cmdEndLabel(cmd);
    }
}

void GameRenderer::cleanup()
{
    const auto& device = gfxDevice.getDevice();

    lightDataBuffer.cleanup(gfxDevice);
    sceneDataBuffer.cleanup(gfxDevice);

    postFXPipeline.cleanup(device);
    depthResolvePipeline.cleanup(device);
    skyboxPipeline.cleanup(device);
    meshPipeline.cleanup(device);
    csmPipeline.cleanup(gfxDevice);
    skinningPipeline.cleanup(gfxDevice);

    gfxDevice.destroyImage(postFXDrawImage);
    gfxDevice.destroyImage(resolveDepthImage);
}

void GameRenderer::updateDevTools(float dt)
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
                onMultisamplingStateUpdate();
            }
        }
        ImGui::EndCombo();
    }
}

bool GameRenderer::isMultisamplingEnabled() const
{
    return samples != VK_SAMPLE_COUNT_1_BIT;
}

void GameRenderer::onMultisamplingStateUpdate()
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

    createDrawImage(gfxDevice.getSwapchainExtent(), false);

    // recreate pipelines
    meshPipeline.init(gfxDevice, drawImageFormat, depthImageFormat, samples);
    skyboxPipeline.init(gfxDevice, drawImageFormat, depthImageFormat, samples);
}

void GameRenderer::setSkyboxImage(ImageId skyboxImageId)
{
    skyboxPipeline.setSkyboxImage(skyboxImageId);
}

void GameRenderer::beginDrawing()
{
    meshDrawCommands.clear();
    lightDataCPU.clear();
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
        sunlightIndex = (std::uint32_t)lightDataCPU.size();
    }

    GPULightData ld{};

    ld.position = transform.getPosition();
    ld.type = light.getShaderType();
    ld.direction = transform.getLocalFront();
    ld.range = light.range;
    if (light.range == 0) {
        if (light.type == LightType::Point) {
            ld.range = pointLightMaxRange;
        } else if (light.type == LightType::Spot) {
            ld.range = spotLightMaxRange;
        }
    }

    ld.color = glm::vec3{light.color};
    ld.intensity = light.intensity;
    if (light.type == LightType::Directional) {
        ld.intensity = 1.0; // don't have intensity for directional light yet
    }

    ld.scaleOffset.x = light.scaleOffset.x;
    ld.scaleOffset.y = light.scaleOffset.y;

    lightDataCPU.push_back(ld);
}

void GameRenderer::drawMesh(MeshId id, const glm::mat4& transform, bool castShadow)
{
    const auto& mesh = baseRenderer.getMesh(id);
    const auto worldBoundingSphere =
        edge::calculateBoundingSphereWorld(transform, mesh.boundingSphere, false);

    meshDrawCommands.push_back(MeshDrawCommand{
        .meshId = id,
        .transformMatrix = transform,
        .worldBoundingSphere = worldBoundingSphere,
        .castShadow = castShadow,
    });
}

void GameRenderer::drawSkinnedMesh(
    std::span<const MeshId> meshes,
    std::span<const SkinnedMesh> skinnedMeshes,
    const glm::mat4& transform,
    std::span<const glm::mat4> jointMatrices)
{
    const auto startIndex =
        skinningPipeline.appendJointMatrices(jointMatrices, gfxDevice.getCurrentFrameIndex());

    assert(meshes.size() == skinnedMeshes.size());
    for (std::size_t i = 0; i < meshes.size(); ++i) {
        const auto& mesh = baseRenderer.getMesh(meshes[i]);
        assert(mesh.hasSkeleton);

        const auto worldBoundingSphere =
            edge::calculateBoundingSphereWorld(transform, mesh.boundingSphere, true);
        meshDrawCommands.push_back(MeshDrawCommand{
            .meshId = meshes[i],
            .transformMatrix = transform,
            .worldBoundingSphere = worldBoundingSphere,
            .skinnedMesh = &skinnedMeshes[i],
            .jointMatricesStartIndex = (std::uint32_t)startIndex,
        });
    }
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

SkinnedMesh GameRenderer::createSkinnedMesh(MeshId id) const
{
    return baseRenderer.createSkinnedMeshBuffer(id);
}

const GPUImage& GameRenderer::getDrawImage() const
{
    // this is our "real" draw image as far as other systems are concerned
    return postFXDrawImage;
}

VkFormat GameRenderer::getDrawImageFormat() const
{
    return drawImageFormat;
}
