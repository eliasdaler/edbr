#include <edbr/DevTools/ResourcesInspector.h>

#include <edbr/Graphics/ImageCache.h>
#include <edbr/Graphics/MaterialCache.h>
#include <edbr/Util/ImGuiUtil.h>

#include <array>
#include <imgui.h>

#include <vulkan/vk_enum_string_helper.h>

namespace
{
void imagePreview(const GPUImage& image, const glm::vec2 maxSize = glm::vec2{512.f, 512.f})
{
    ImGui::Text("%s (%u, %u)", image.debugName.c_str(), image.extent.width, image.extent.height);
    auto scale = 1.f;
    if ((float)image.extent.width > maxSize.x || (float)image.extent.height > maxSize.y) {
        scale = std::min(maxSize.x / image.extent.width, maxSize.y / image.extent.height);
    }
    util::ImGuiImage(image, scale);
}
}

void ResourcesInspector::update(
    float dt,
    const ImageCache& imageCache,
    const MaterialCache& materialCache)
{
    ImGui::Begin("Resources");

    static bool showPreviewOnHover = true;
    ImGui::Checkbox("Preview on hover", &showPreviewOnHover);
    static ImageId selectedImageId{NULL_IMAGE_ID};

    static bool showPreviewWindow = true;
    static bool showFormatColumn = false;
    static bool showUsageColumn = false;
    if (ImGui::TreeNode("Images")) {
        ImGui::Checkbox("Format", &showFormatColumn);
        ImGui::SameLine();
        ImGui::Checkbox("Usage", &showUsageColumn);

        static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders |
                                       ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
        int numColumns = 3;
        if (showFormatColumn) {
            ++numColumns;
        }
        if (showUsageColumn) {
            ++numColumns;
        }

        if (ImGui::BeginTable("Images", numColumns, flags)) {
            ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Size");
            if (showFormatColumn) {
                ImGui::TableSetupColumn("Format", ImGuiTableColumnFlags_WidthFixed);
            }
            if (showUsageColumn) {
                ImGui::TableSetupColumn("Usage");
            }
            ImGui::TableHeadersRow();

            for (const auto& image : imageCache.images) {
                if (!image.isInitialized()) {
                    continue;
                }
                ImGui::PushID((int)image.getBindlessId());
                ImGui::TableNextColumn();
                ImGui::Text("%u", image.getBindlessId());

                ImGui::TableNextColumn();
                if (ImGui::Selectable(
                        image.debugName.c_str(), image.getBindlessId() == selectedImageId)) {
                    selectedImageId = image.getBindlessId();
                    showPreviewWindow = true;
                };

                if (showPreviewOnHover && ImGui::IsItemHovered()) {
                    if (ImGui::BeginItemTooltip()) {
                        imagePreview(image);
                        ImGui::EndTooltip();
                    }
                }

                ImGui::TableNextColumn();
                ImGui::Text("(%u, %u)", image.extent.width, image.extent.height);

                if (showFormatColumn) {
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(string_VkFormat(image.format));
                }

                if (showUsageColumn) {
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(string_VkImageUsageFlags(image.usage).c_str());
                }

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::TreePop();
    }

    const auto displayColor = [](glm::vec4 color) {
        const auto flags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoInputs;
        std::array<float, 4> arr{color.x, color.y, color.z, color.w};
        ImGui::ColorEdit3("##Color", arr.data(), flags);
    };

    if (ImGui::TreeNode("Materials")) {
        static ImGuiTableFlags flags =
            ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
        if (ImGui::BeginTable("Images", 10, flags)) {
            ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Base col", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Metal", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Rough", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Emissive", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("D tex", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("N tex", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("MR tex", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("E tex", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            std::uint32_t materialId = 0;
            for (const auto& material : materialCache.materials) {
                // TODO: somehow check if material is intialized or not?
                ImGui::PushID((int)materialId);
                ImGui::TableNextColumn();
                ImGui::Text("%u", materialId);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(material.name.c_str());

                ImGui::TableNextColumn();
                displayColor(material.baseColor);

                ImGui::TableNextColumn();
                ImGui::Text("%.2f", material.metallicFactor);

                ImGui::TableNextColumn();
                ImGui::Text("%.2f", material.roughnessFactor);

                ImGui::TableNextColumn();
                ImGui::Text("%.2f", material.emissiveFactor);

                const auto displayImage = [&](ImageId id) {
                    if (id == NULL_IMAGE_ID) {
                        return;
                    }
                    ImGui::PushID((int)id);
                    if (ImGui::Selectable("*", selectedImageId == id)) {
                        selectedImageId = id;
                        showPreviewWindow = true;
                    }
                    if (showPreviewOnHover && ImGui::IsItemHovered()) {
                        if (ImGui::BeginItemTooltip()) {
                            const auto image = imageCache.getImage(id);
                            imagePreview(image);
                            ImGui::EndTooltip();
                        }
                    }
                    ImGui::PopID();
                };

                ImGui::TableNextColumn();
                displayImage(material.diffuseTexture);

                ImGui::TableNextColumn();
                displayImage(material.normalMapTexture);

                ImGui::TableNextColumn();
                displayImage(material.metallicRoughnessTexture);

                ImGui::TableNextColumn();
                displayImage(material.emissiveTexture);

                ++materialId;
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Meshes")) {
        ImGui::TextUnformatted("TODO");
        ImGui::TreePop();
    }
    ImGui::End();

    if (selectedImageId != NULL_IMAGE_ID && showPreviewWindow) {
        auto& image = imageCache.getImage(selectedImageId);
        if (ImGui::Begin("Image preview", &showPreviewWindow, ImGuiWindowFlags_NoResize)) {
            ImGui::SetWindowSize({540, 600});
            imagePreview(image);
        }
        ImGui::End();
    }
}
