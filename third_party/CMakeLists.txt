# glm
add_subdirectory(glm)

# glfw
add_subdirectory(glfw)
add_library(glfw::glfw ALIAS glfw)

# vkb
add_subdirectory(vk-bootstrap)
# vma
add_subdirectory(vma)

## Dear ImGui
set(IMGUI_SOURCES
  "imgui.cpp"
  "imgui_draw.cpp"
  "imgui_tables.cpp"
  "imgui_widgets.cpp"
  "imgui_demo.cpp"
  # C++ additions
  "misc/cpp/imgui_stdlib.cpp"
  # backend
  "backends/imgui_impl_vulkan.cpp"
  "backends/imgui_impl_glfw.cpp"
)

list(TRANSFORM IMGUI_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/imgui/")

add_library(imgui STATIC ${IMGUI_SOURCES})
set_property(TARGET imgui PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(imgui
  PUBLIC
    "imgui"
    "imgui/backends"
)

find_package(Vulkan REQUIRED)

target_link_libraries(imgui PUBLIC
  glfw::glfw
  Vulkan::Vulkan
)
add_library(imgui::imgui ALIAS imgui)

## tinygltf
option(TINYGLTF_BUILD_LOADER_EXAMPLE "Build loader_example(load glTF and dump infos)" OFF)
option(TINYGLTF_INSTALL "Install tinygltf files during install step." OFF)
option(TINYGLTF_HEADER_ONLY "On: header-only mode. Off: create tinygltf library(No TINYGLTF_IMPLEMENTATION required in your project)" ON)
add_subdirectory(tinygltf)
add_library(tinygltf::tinygltf ALIAS tinygltf)
