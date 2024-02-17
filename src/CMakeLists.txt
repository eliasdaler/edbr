add_executable(game
  DeletionQueue.cpp
  VkInit.cpp
  VkUtil.cpp

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
)

set(SHADERS
  gradient.comp
)
list(TRANSFORM SHADERS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/shaders/")

target_shaders(game ${SHADERS})
