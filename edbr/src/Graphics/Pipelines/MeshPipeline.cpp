#include <edbr/Graphics/Pipelines/MeshPipeline.h>

#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/MeshDrawCommand.h>

#include <edbr/Graphics/BaseRenderer.h>
#include <edbr/Graphics/FrustumCulling.h>
#include <edbr/Graphics/Vulkan/Init.h>
#include <edbr/Graphics/Vulkan/Pipelines.h>
#include <edbr/Graphics/Vulkan/Util.h>

void MeshPipeline::init(
    GfxDevice& gfxDevice,
    VkFormat drawImageFormat,
    VkFormat depthImageFormat,
    VkSampleCountFlagBits samples)
{
    const auto& device = gfxDevice.getDevice();

    const auto vertexShader = vkutil::loadShaderModule("shaders/mesh.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/mesh.frag.spv", device);

    vkutil::addDebugLabel(device, vertexShader, "mesh.vert");
    vkutil::addDebugLabel(device, vertexShader, "mesh.frag");

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts = std::array{gfxDevice.getBindlessDescSetLayout()};
    pipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);
    vkutil::addDebugLabel(device, pipelineLayout, "mesh pipeline layout");

    pipeline = PipelineBuilder{pipelineLayout}
                   .setShaders(vertexShader, fragShader)
                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                   .enableCulling()
                   .setMultisampling(samples)
                   .disableBlending()
                   .setColorAttachmentFormat(drawImageFormat)
                   .setDepthFormat(depthImageFormat)
                   .enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                   .build(device);
    vkutil::addDebugLabel(device, pipeline, "mesh pipeline");

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);
}

void MeshPipeline::draw(
    VkCommandBuffer cmd,
    VkExtent2D renderExtent,
    const GfxDevice& gfxDevice,
    const BaseRenderer& baseRenderer,
    const Camera& camera,
    const GPUBuffer& sceneDataBuffer,
    const std::vector<MeshDrawCommand>& drawCommands,
    const std::vector<std::size_t>& sortedDrawCommands)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    gfxDevice.bindBindlessDescSet(cmd, pipelineLayout);

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

        const auto& mesh = baseRenderer.getMesh(dc.meshId);
        if (mesh.materialId != prevMaterialIdx) {
            prevMaterialIdx = mesh.materialId;
        }

        if (dc.meshId != prevMeshId) {
            prevMeshId = dc.meshId;
            vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        }

        const auto pushConstants = PushConstants{
            .transform = dc.transformMatrix,
            .sceneDataBuffer = sceneDataBuffer.address,
            .vertexBuffer = dc.skinnedMesh ? dc.skinnedMesh->skinnedVertexBuffer.address :
                                             mesh.vertexBuffer.address,
            .materialId = (std::uint32_t)mesh.materialId,
        };
        vkCmdPushConstants(
            cmd,
            pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PushConstants),
            &pushConstants);

        vkCmdDrawIndexed(cmd, mesh.numIndices, 1, 0, 0, 0);
    }
}

void MeshPipeline::cleanup(VkDevice device)
{
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
}
