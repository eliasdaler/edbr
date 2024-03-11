#include "DrawableString.h"

#include <Graphics/Font.h>
#include <Graphics/GfxDevice.h>
#include <Graphics/Vulkan/Descriptors.h>
#include <Graphics/Vulkan/Util.h>

#include <array>
#include <vector>

#include <utf8.h>

namespace
{
/*
 *   0   2
 *   ┌───┐
 *   │  ╱│
 *   │ ╱ │
 *   │╱  │
 *   └───┘
 *   1   3
 */
constexpr std::array<std::size_t, 6> indicesOrder{0, 1, 2, 2, 3, 1};

struct Vertex {
    Vertex(glm::vec2 pos, glm::vec2 uv, glm::vec3 color = glm::vec3(1.f, 1.f, 1.f)) :
        pos(pos, 0.f), uv_x(uv.x), color(color), uv_y(uv.y)
    {}

    glm::vec3 pos;
    float uv_x;
    glm::vec3 color;
    float uv_y;
};

void appendQuad(
    const glm::vec2& pos,
    const glm::vec2& size,
    const glm::vec2& uv1,
    const glm::vec2& uv2,
    const glm::vec3& color,
    std::vector<Vertex>& vertices,
    std::vector<std::uint32_t>& indices)
{
    const auto i = vertices.size(); // index of fist added vertex

    vertices.push_back(Vertex(pos, uv1, color)); // top-left
    vertices.push_back(
        Vertex(glm::vec2{pos.x, pos.y + size.y}, {uv1.x, uv2.y}, color)); // bottom-left
    vertices.push_back(
        Vertex(glm::vec2{pos.x + size.x, pos.y}, {uv2.x, uv1.y}, color)); // top-right
    vertices.push_back(
        Vertex(glm::vec2{pos.x + size.x, pos.y + size.y}, uv2, color)); // bottom-right

    for (const auto j : indicesOrder) {
        indices.push_back(static_cast<std::uint32_t>(i + j));
    }
}

void uploadToGPU(
    const GfxDevice& gfxDevice,
    DrawableString& str,
    const std::vector<Vertex>& vertices,
    const std::vector<std::uint32_t>& indices)
{
    const auto indexBufferSize = indices.size() * sizeof(std::uint32_t);
    str.indexBuffer = gfxDevice.createBuffer(
        indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    str.numIndices = indices.size();

    const auto vertexBufferSize = vertices.size() * sizeof(Vertex);
    str.vertexBuffer = gfxDevice.createBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

    const auto staging =
        gfxDevice
            .createBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    // copy data
    void* data = staging.info.pMappedData;
    memcpy(data, vertices.data(), vertexBufferSize);
    memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

    gfxDevice.immediateSubmit([&](VkCommandBuffer cmd) {
        const auto vertexCopy = VkBufferCopy{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = vertexBufferSize,
        };
        vkCmdCopyBuffer(cmd, staging.buffer, str.vertexBuffer.buffer, 1, &vertexCopy);

        const auto indexCopy = VkBufferCopy{
            .srcOffset = vertexBufferSize,
            .dstOffset = 0,
            .size = indexBufferSize,
        };
        vkCmdCopyBuffer(cmd, staging.buffer, str.indexBuffer.buffer, 1, &indexCopy);
    });

    gfxDevice.destroyBuffer(staging);
}

} // end of anonymous namespace

void DrawableString::init(
    GfxDevice& gfxDevice,
    std::string text,
    const Font& font,
    VkSampler linearSampler,
    VkDescriptorSetLayout uiElementDescSetLayout)
{
    this->font = &font;

    float x = 0.0f;
    int lineNum = 0;

    if (initialized) {
        // this might be slow if there's a lot of string updating going on...
        // but will do for now
        gfxDevice.waitIdle();
        cleanup(gfxDevice);
    }

    if (!initialized || this->font != &font) {
        uiElementDescSet = gfxDevice.allocateDescriptorSet(uiElementDescSetLayout);

        DescriptorWriter writer;
        writer.writeImage(
            0,
            font.glyphAtlas.imageView,
            linearSampler,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.updateSet(gfxDevice.getDevice(), uiElementDescSet);
    }

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    auto it = text.begin();
    const auto e = text.end();
    std::uint32_t cp = 0;
    while (it != e) {
        cp = utf8::next(it, e);

        if (cp == static_cast<std::uint32_t>('\n')) {
            ++lineNum;
            x = 0.f;
            continue;
        }

        const auto& glyph = font.glyphs.contains(cp) ? font.glyphs.at(cp) : font.glyphs.at('?');
        const auto& uv0 = glyph.uv0;
        const auto& uv1 = glyph.uv1;

        float xpos = x + glyph.bearing.x;
        float ypos = -glyph.bearing.y + static_cast<float>(lineNum) * font.lineSpacing;
        auto s = (uv1 - uv0) * font.getGlyphAtlasSize();

        appendQuad({xpos, ypos}, s, uv0, uv1, glm::vec3{1.f, 1.f, 1.f}, vertices, indices);
        x += glyph.advance;
    }

    uploadToGPU(gfxDevice, *this, vertices, indices);
    vkutil::addDebugLabel(gfxDevice.getDevice(), vertexBuffer.buffer, "lmao");

    this->text = std::move(text);
    initialized = true;
}

void DrawableString::cleanup(const GfxDevice& gfxDevice)
{
    if (initialized) {
        gfxDevice.destroyBuffer(vertexBuffer);
        gfxDevice.destroyBuffer(indexBuffer);
        initialized = false;
    }
}
