add_executable(game
  Graphics/Camera.cpp
  Graphics/Cubemap.cpp
  Graphics/FrustumCulling.cpp
  Graphics/GfxDevice.cpp
  Graphics/MaterialCache.cpp
  Graphics/ImageCache.cpp
  Graphics/MeshCache.cpp
  Graphics/ShadowMapping.cpp
  Graphics/SkeletonAnimator.cpp
  Graphics/Swapchain.cpp
  Graphics/Texture.cpp

  Graphics/BaseRenderer.cpp
  Graphics/GameRenderer.cpp

  Graphics/Pipelines/CSMPipeline.cpp
  Graphics/Pipelines/DepthResolvePipeline.cpp
  Graphics/Pipelines/MeshPipeline.cpp
  Graphics/Pipelines/PostFXPipeline.cpp
  Graphics/Pipelines/SkinningPipeline.cpp
  Graphics/Pipelines/SkyboxPipeline.cpp

  Graphics/Vulkan/DeletionQueue.cpp
  Graphics/Vulkan/Descriptors.cpp
  Graphics/Vulkan/Init.cpp
  Graphics/Vulkan/NBuffer.cpp
  Graphics/Vulkan/Pipelines.cpp
  Graphics/Vulkan/Util.cpp
  Graphics/Vulkan/VmaImpl.cpp
  Graphics/Vulkan/VulkanImGui.cpp
  Graphics/Vulkan/VulkanImmediateExecutor.cpp

  util/GltfLoader.cpp
  util/InputUtil.cpp
  util/ImageLoader.cpp
  util/OSUtil.cpp

  Math/Transform.cpp
  Math/Util.cpp

  FreeCameraController.cpp

  Game.cpp

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
  # Vulkan::Vulkan
  volk::volk
  vk-bootstrap::vk-bootstrap
  GPUOpen::VulkanMemoryAllocator

  imgui::imgui
  glm::glm

  tinygltf::tinygltf
  stb::image
)

target_compile_definitions(game
  PUBLIC
    VK_NO_PROTOTYPES
    VMA_VULKAN_VERSION=1003000
)


# SDL
if(BUILD_SHARED_LIBS)
  target_link_libraries(game PRIVATE
    SDL2::SDL2
  )
else()
  target_link_libraries(game PRIVATE
    SDL2::SDL2-static
  )
endif()

if(WIN32)
  target_link_libraries(game PRIVATE SDL2::SDL2main)
endif()

# glm
target_compile_definitions(game
  PUBLIC
    GLM_FORCE_CTOR_INIT
    GLM_FORCE_XYZW_ONLY
    GLM_FORCE_EXPLICIT_CTOR
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
)

set(SHADERS
  fullscreen_triangle.vert
  skybox.frag
  skinning.comp
  mesh.vert
  mesh_depth_only.vert
  mesh.frag
  postfx.frag
  depth_resolve.frag
)
list(TRANSFORM SHADERS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/shaders/")

target_shaders(game ${SHADERS})

add_custom_command(TARGET game POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "Copying game assets to $<TARGET_FILE_DIR:game>/assets"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:game>/assets"
)

target_link_libraries(game PUBLIC Tracy::TracyClient)

if (MSVC)
  target_compile_definitions(TracyClient PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:TRACY_ENABLE>)
  target_compile_definitions(game PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:TRACY_ENABLE>)
else()
  if(NOT CMAKE_BUILD_TYPE OR
    CMAKE_BUILD_TYPE STREQUAL "Debug" OR
    CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(TracyClient PRIVATE TRACY_ENABLE)
    target_compile_definitions(game PRIVATE TRACY_ENABLE)
  endif()
endif()
