add_executable(game
  DeletionQueue.cpp
  VkInit.cpp
  VkUtil.cpp
  VkDescriptors.cpp
  VkPipelines.cpp

  OSUtil.cpp

  Renderer.cpp

  main.cpp
)

set_target_properties(game PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
)

target_add_extra_warnings(game)

target_include_directories(game PRIVATE "${CMAKE_CURRENT_LIST_DIR}")

find_package(Vulkan REQUIRED)

target_link_libraries(game PRIVATE
  glfw::glfw

  Vulkan::Vulkan
  vk-bootstrap::vk-bootstrap
  GPUOpen::VulkanMemoryAllocator

  imgui::imgui
  glm::glm
)

# glm
target_compile_definitions(game
  PUBLIC
    GLM_FORCE_CTOR_INIT
    GLM_FORCE_XYZW_ONLY
    GLM_FORCE_EXPLICIT_CTOR
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

set(SHADERS
  colored_triangle.vert
  colored_triangle.frag
  mesh.vert
  mesh.frag
  gradient.comp
)
list(TRANSFORM SHADERS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/shaders/")

target_shaders(game ${SHADERS})
