#include "GameRenderer.h"

#include <Graphics/Cubemap.h>
#include <Graphics/FrustumCulling.h>
#include <Graphics/Scene.h>
#include <Graphics/Vulkan/Init.h>
#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>
#include <util/GltfLoader.h>

#include <imgui.h>

#include <numeric> // iota

GameRenderer::GameRenderer(GfxDevice& gfxDevice) : gfxDevice(gfxDevice), baseRenderer(gfxDevice)
{}

void GameRenderer::init()
{
    baseRenderer.init();
    initSceneData();

    samples = gfxDevice.getMaxSupportedSamplingCount(); // needs to be called before
                                                        // createDrawImage
    createDrawImage(gfxDevice.getSwapchainExtent(), true);

    skinningPipeline.init(gfxDevice);
    csmPipeline.init(gfxDevice, std::array{0.08f, 0.2f, 0.5f});
    meshPipeline.init(
        gfxDevice,
        drawImage.format,
        depthImage.format,
        sceneDataDescriptorLayout,
        baseRenderer.getMaterialDataDescSetLayout(),
        samples);

    depthResolvePipeline.init(gfxDevice, depthImage);
    depthResolvePipeline
        .setDepthImage(gfxDevice, depthImage, baseRenderer.getDefaultNearestSampler());

    skyboxPipeline.init(gfxDevice, drawImage.format, depthImage.format, samples);
    postFXPipeline.init(gfxDevice, drawImage.format, sceneDataDescriptorLayout);

    if (isMultisamplingEnabled()) {
        postFXPipeline.setImages(
            gfxDevice, resolveImage, resolveDepthImage, baseRenderer.getDefaultNearestSampler());
    } else {
        postFXPipeline
            .setImages(gfxDevice, drawImage, depthImage, baseRenderer.getDefaultNearestSampler());
    }

    skyboxImage = graphics::loadCubemap(gfxDevice, "assets/images/skybox/distant_sunset");
    skyboxPipeline.setSkyboxImage(gfxDevice, skyboxImage, baseRenderer.getDefaultLinearSampler());

    // depends on csm pipeline being created
    updateSceneDataDescriptorSet();
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
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = usages,
            .extent = drawImageExtent,
            .samples = samples,
        };
        drawImage = gfxDevice.createImage(createImageInfo);

        vkutil::addDebugLabel(gfxDevice.getDevice(), drawImage.image, "draw image");
        vkutil::addDebugLabel(gfxDevice.getDevice(), drawImage.imageView, "draw image view");

        if (firstCreate) {
            createImageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // no MSAA
            postFXDrawImage = gfxDevice.createImage(createImageInfo);
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
        resolveImage = gfxDevice.createImage(createImageInfo);

        vkutil::addDebugLabel(gfxDevice.getDevice(), resolveImage.image, "resolve image");
        vkutil::addDebugLabel(gfxDevice.getDevice(), resolveImage.imageView, "resolve image");
    }

    { // setup depth image
        auto createInfo = vkutil::CreateImageInfo{
            .format = VK_FORMAT_D32_SFLOAT,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .extent = drawImageExtent,
            .samples = samples,
        };

        depthImage = gfxDevice.createImage(createInfo);
        vkutil::addDebugLabel(gfxDevice.getDevice(), depthImage.image, "draw image (depth)");
        vkutil::addDebugLabel(gfxDevice.getDevice(), depthImage.imageView, "draw image depth view");

        if (firstCreate) {
            createInfo.samples = VK_SAMPLE_COUNT_1_BIT; // NO MSAA
            resolveDepthImage = gfxDevice.createImage(createInfo);

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
    const auto sceneDataBindings = std::array<DescriptorLayoutBinding, 2>{{
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    }};
    sceneDataDescriptorLayout = vkutil::buildDescriptorSetLayout(
        gfxDevice.getDevice(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        sceneDataBindings);

    sceneDataBuffer.init(
        gfxDevice.getDevice(),
        gfxDevice.getAllocator(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        sizeof(GPUSceneData),
        graphics::FRAME_OVERLAP,
        "scene data");

    sceneDataDescriptorSet = gfxDevice.allocateDescriptorSet(sceneDataDescriptorLayout);
}

void GameRenderer::updateSceneDataDescriptorSet()
{
    DescriptorWriter writer;
    writer.writeBuffer(
        0,
        sceneDataBuffer.getBuffer().buffer,
        sizeof(GPUSceneData),
        0,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeImage(
        1,
        csmPipeline.getShadowMap().imageView,
        baseRenderer.getDefaultShadowMapSampler(),
        VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(gfxDevice.getDevice(), sceneDataDescriptorSet);
}

void GameRenderer::draw(const Camera& camera, const SceneData& sceneData)
{
    auto cmd = gfxDevice.beginFrame();
    draw(cmd, camera, sceneData);

    // we'll copy from post FX draw image to swapchain
    vkutil::transitionImage(
        cmd,
        postFXDrawImage.image,
        VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    gfxDevice.endFrame(cmd, postFXDrawImage);
}

void GameRenderer::draw(VkCommandBuffer cmd, const Camera& camera, const SceneData& sceneData)
{
    { // skinning
        vkutil::cmdBeginLabel(cmd, "Skinning");
        for (const auto& dc : drawCommands) {
            if (!dc.skinnedMesh) {
                continue;
            }
            skinningPipeline.doSkinning(cmd, gfxDevice.getCurrentFrameIndex(), baseRenderer, dc);
        }
        vkutil::cmdEndLabel(cmd);

        // Sync skinning with CSM
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

    { // CSM
        ZoneScopedN("CSM");
        TracyVkZoneC(gfxDevice.getTracyVkCtx(), cmd, "CSM", tracy::Color::CornflowerBlue);
        vkutil::cmdBeginLabel(cmd, "CSM");

        vkutil::transitionImage(
            cmd,
            csmPipeline.getShadowMap().image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        csmPipeline.draw(
            cmd,
            baseRenderer,
            camera,
            glm::vec3{sceneData.sunlightDirection},
            drawCommands,
            shadowsEnabled);

        vkutil::cmdEndLabel(cmd);

        // Sync CSM with geometry pass
        const auto imageBarrier = VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
            .image = csmPipeline.getShadowMap().image,
            .subresourceRange = vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT),
        };
        const auto dependencyInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageBarrier,
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }

    { // upload scene data - can only be done after CSM has finished
        const auto gpuSceneData = GPUSceneData{
            .view = sceneData.camera.getView(),
            .proj = sceneData.camera.getProjection(),
            .viewProj = sceneData.camera.getViewProj(),
            .cameraPos = glm::vec4{sceneData.camera.getPosition(), 1.f},
            .ambientColorAndIntensity = sceneData.ambientColorAndIntensity,
            .sunlightDirection = sceneData.sunlightDirection,
            .sunlightColorAndIntensity = sceneData.sunlightColorAndIntensity,
            .fogColorAndDensity = sceneData.fogColorAndDensity,
            .cascadeFarPlaneZs =
                glm::vec4{
                    csmPipeline.cascadeFarPlaneZs[0],
                    csmPipeline.cascadeFarPlaneZs[1],
                    csmPipeline.cascadeFarPlaneZs[2],
                    0.f,
                },
            .csmLightSpaceTMs = csmPipeline.csmLightSpaceTMs,
        };
        sceneDataBuffer.uploadNewData(
            cmd, gfxDevice.getCurrentFrameIndex(), (void*)&gpuSceneData, sizeof(GPUSceneData));
    }

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
            .colorImageClearValue = glm::vec4{0.f, 0.f, 0.f, 0.f},
            .depthImageView = depthImage.imageView,
            .depthImageClearValue = 0.f,
            .resolveImageView = isMultisamplingEnabled() ? resolveImage.imageView : VK_NULL_HANDLE,
        });

        vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

        meshPipeline.draw(
            cmd,
            drawImage.getExtent2D(),
            baseRenderer,
            camera,
            sceneDataDescriptorSet,
            drawCommands,
            sortedDrawCommands);

        // sky
        skyboxPipeline.draw(cmd, camera);

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

        depthResolvePipeline.draw(cmd, depthImage.getExtent2D(), vkutil::sampleCountToInt(samples));

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
        postFXPipeline.draw(cmd, sceneDataDescriptorSet);
        vkCmdEndRendering(cmd);

        vkutil::cmdEndLabel(cmd);
    }
}

void GameRenderer::cleanup()
{
    const auto& device = gfxDevice.getDevice();

    vkDeviceWaitIdle(device);

    sceneDataBuffer.cleanup(device, gfxDevice.getAllocator());
    vkDestroyDescriptorSetLayout(device, sceneDataDescriptorLayout, nullptr);

    postFXPipeline.cleanup(device);
    depthResolvePipeline.cleanup(device);
    skyboxPipeline.cleanup(device);
    meshPipeline.cleanup(device);
    csmPipeline.cleanup(gfxDevice);
    skinningPipeline.cleanup(gfxDevice);

    gfxDevice.destroyImage(skyboxImage);

    gfxDevice.destroyImage(postFXDrawImage);
    gfxDevice.destroyImage(resolveDepthImage);
    gfxDevice.destroyImage(depthImage);
    gfxDevice.destroyImage(resolveImage);
    gfxDevice.destroyImage(drawImage);

    baseRenderer.cleanup();
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
    vkDeviceWaitIdle(gfxDevice.getDevice());

    { // cleanup old state
        meshPipeline.cleanup(gfxDevice.getDevice());
        skyboxPipeline.cleanup(gfxDevice.getDevice());

        gfxDevice.destroyImage(depthImage);
        gfxDevice.destroyImage(drawImage);
    }

    // recreate pipelines
    meshPipeline.init(
        gfxDevice,
        drawImage.format,
        depthImage.format,
        sceneDataDescriptorLayout,
        baseRenderer.getMaterialDataDescSetLayout(),
        samples);
    skyboxPipeline.init(gfxDevice, drawImage.format, depthImage.format, samples);
    skyboxPipeline.setSkyboxImage(gfxDevice, skyboxImage, baseRenderer.getDefaultLinearSampler());

    createDrawImage(gfxDevice.getSwapchainExtent(), false);

    if (isMultisamplingEnabled()) {
        depthResolvePipeline
            .setDepthImage(gfxDevice, depthImage, baseRenderer.getDefaultNearestSampler());
        postFXPipeline.setImages(
            gfxDevice, resolveImage, resolveDepthImage, baseRenderer.getDefaultNearestSampler());
    } else {
        postFXPipeline
            .setImages(gfxDevice, drawImage, depthImage, baseRenderer.getDefaultNearestSampler());
    }
}

Scene GameRenderer::loadScene(const std::filesystem::path& path)
{
    util::LoadContext loadContext{
        .renderer = baseRenderer,
    };
    util::SceneLoader loader;

    Scene scene;
    loader.loadScene(loadContext, scene, path);
    return scene;
}

void GameRenderer::beginDrawing()
{
    drawCommands.clear();
    skinningPipeline.beginDrawing(gfxDevice.getCurrentFrameIndex());
}

void GameRenderer::endDrawing()
{
    sortDrawList();
}

void GameRenderer::addDrawCommand(MeshId id, const glm::mat4& transform, bool castShadow)
{
    const auto& mesh = baseRenderer.getMesh(id);
    const auto worldBoundingSphere =
        edge::calculateBoundingSphereWorld(transform, mesh.boundingSphere, false);

    drawCommands.push_back(DrawCommand{
        .meshId = id,
        .transformMatrix = transform,
        .worldBoundingSphere = worldBoundingSphere,
        .castShadow = castShadow,
    });
}

void GameRenderer::addDrawSkinnedMeshCommand(
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
        drawCommands.push_back(DrawCommand{
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
    sortedDrawCommands.clear();
    sortedDrawCommands.resize(drawCommands.size());
    std::iota(sortedDrawCommands.begin(), sortedDrawCommands.end(), 0);

    std::sort(
        sortedDrawCommands.begin(),
        sortedDrawCommands.end(),
        [this](const auto& i1, const auto& i2) {
            const auto& dc1 = drawCommands[i1];
            const auto& dc2 = drawCommands[i2];
            const auto& mesh1 = baseRenderer.getMesh(dc1.meshId);
            const auto& mesh2 = baseRenderer.getMesh(dc2.meshId);
            if (mesh1.materialId == mesh2.materialId) {
                return dc1.meshId < dc2.meshId;
            }
            return mesh1.materialId < mesh2.materialId;
        });
}

SkinnedMesh GameRenderer::createSkinnedMesh(MeshId id) const
{
    return baseRenderer.createSkinnedMeshBuffer(id);
}
