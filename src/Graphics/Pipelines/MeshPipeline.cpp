#include "MeshPipeline.h"

#include <Graphics/DrawCommand.h>
#include <Graphics/Renderer.h>

#include <Graphics/FrustumCulling.h>
#include <Graphics/Vulkan/Descriptors.h>
#include <Graphics/Vulkan/Init.h>
#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>

MeshPipeline::MeshPipeline(
    Renderer& renderer,
    VkFormat drawImageFormat,
    VkFormat depthImageFormat,
    VkSampleCountFlagBits samples) :
    renderer(renderer)
{
    const auto& device = renderer.getDevice();

    const auto vertexShader = vkutil::loadShaderModule("shaders/mesh.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/mesh.frag.spv", device);

    vkutil::addDebugLabel(device, vertexShader, "mesh.vert");
    vkutil::addDebugLabel(device, vertexShader, "mesh.frag");

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(GPUDrawPushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts =
        std::array{renderer.getSceneDataDescSetLayout(), renderer.getMaterialDataDescSetLayout()};
    meshPipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);

    meshPipeline = PipelineBuilder{meshPipelineLayout}
                       .setShaders(vertexShader, fragShader)
                       .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                       .setPolygonMode(VK_POLYGON_MODE_FILL)
                       .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                       .setMultisampling(samples)
                       .disableBlending()
                       .setColorAttachmentFormat(drawImageFormat)
                       .setDepthFormat(depthImageFormat)
                       .enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                       .build(device);
    vkutil::addDebugLabel(device, meshPipeline, "mesh pipeline");

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);
}

void MeshPipeline::draw(
    VkCommandBuffer cmd,
    VkExtent2D renderExtent,
    const Camera& camera,
    VkDescriptorSet sceneDataDescriptorSet,
    const std::vector<DrawCommand>& drawCommands,
    const std::vector<std::size_t>& sortedDrawCommands)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        meshPipelineLayout,
        0,
        1,
        &sceneDataDescriptorSet,
        0,
        nullptr);

    const auto viewport = VkViewport{
        .x = 0,
        .y = 0,
        .width = (float)renderExtent.width,
        .height = (float)renderExtent.height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    const auto scissor = VkRect2D{
        .offset = {},
        .extent = renderExtent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    auto prevMaterialIdx = NULL_MATERIAL_ID;
    auto prevMeshId = NULL_MESH_ID;

    const auto frustum = edge::createFrustumFromCamera(camera);

    for (const auto& dcIdx : sortedDrawCommands) {
        const auto& dc = drawCommands[dcIdx];
        if (!edge::isInFrustum(frustum, dc.worldBoundingSphere)) {
            continue;
        }

        const auto& mesh = renderer.getMesh(dc.meshId);
        if (mesh.materialId != prevMaterialIdx) {
            prevMaterialIdx = mesh.materialId;

            const auto& material = renderer.getMaterial(mesh.materialId);
            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                meshPipelineLayout,
                1,
                1,
                &material.materialSet,
                0,
                nullptr);
        }

        if (dc.meshId != prevMeshId) {
            prevMeshId = dc.meshId;
            vkCmdBindIndexBuffer(cmd, mesh.buffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        }

        const auto pushConstants = GPUDrawPushConstants{
            .transform = dc.transformMatrix,
            .vertexBuffer = dc.skinnedMesh ? dc.skinnedMesh->skinnedVertexBufferAddress :
                                             mesh.buffers.vertexBufferAddress,
        };
        vkCmdPushConstants(
            cmd,
            meshPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(GPUDrawPushConstants),
            &pushConstants);

        vkCmdDrawIndexed(cmd, mesh.numIndices, 1, 0, 0, 0);
    }
}

void MeshPipeline::cleanup(VkDevice device)
{
    vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
    vkDestroyPipeline(device, meshPipeline, nullptr);
}
