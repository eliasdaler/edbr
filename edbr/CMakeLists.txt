add_subdirectory(third_party)

add_library(edbr
  # Core
  src/Core/JsonDataLoader.cpp
  src/Core/JsonFile.cpp
  src/Core/JsonMath.cpp

  # DevTools
  src/DevTools/EntityInfoDisplayer.cpp
  src/DevTools/EntityTreeView.cpp
  src/DevTools/ImGuiPropertyTable.cpp
  src/DevTools/Im3dState.cpp

  # ECS
  src/ECS/ComponentFactory.cpp
  src/ECS/EntityFactory.cpp

  # Math
  src/Math/Transform.cpp
  src/Math/Util.cpp

  # Graphics/Vulkan
  src/Graphics/Vulkan/BindlessSetManager.cpp
  src/Graphics/Vulkan/Descriptors.cpp
  src/Graphics/Vulkan/Init.cpp
  src/Graphics/Vulkan/Pipelines.cpp
  src/Graphics/Vulkan/Swapchain.cpp
  src/Graphics/Vulkan/Util.cpp
  src/Graphics/Vulkan/VmaImpl.cpp
  src/Graphics/Vulkan/VolkImpl.cpp
  src/Graphics/Vulkan/VulkanImGuiBackend.cpp
  src/Graphics/Vulkan/VulkanImmediateExecutor.cpp

  # Graphics
  src/Graphics/Camera.cpp
  src/Graphics/Cubemap.cpp
  src/Graphics/Font.cpp
  src/Graphics/FrustumCulling.cpp
  src/Graphics/GfxDevice.cpp
  src/Graphics/ImageCache.cpp
  src/Graphics/ImageLoader.cpp
  src/Graphics/MaterialCache.cpp
  src/Graphics/MeshCache.cpp
  src/Graphics/MipMapGeneration.cpp
  src/Graphics/NBuffer.cpp
  src/Graphics/ShadowMapping.cpp
  src/Graphics/SkeletonAnimator.cpp
  src/Graphics/Sprite.cpp

  # Graphics/Pipeline
  src/Graphics/Pipelines/CSMPipeline.cpp
  src/Graphics/Pipelines/DepthResolvePipeline.cpp
  src/Graphics/Pipelines/MeshPipeline.cpp
  src/Graphics/Pipelines/PostFXPipeline.cpp
  src/Graphics/Pipelines/SkinningPipeline.cpp
  src/Graphics/Pipelines/SkyboxPipeline.cpp
  src/Graphics/Pipelines/SpriteDrawingPipeline.cpp

  # Graphics - high level renderers
  src/Graphics/BaseRenderer.cpp
  src/Graphics/GameRenderer.cpp
  src/Graphics/SpriteRenderer.cpp

  # Util
  src/Util/GltfLoader.cpp
  src/Util/InputUtil.cpp
  src/Util/ImGuiUtil.cpp
  src/Util/OSUtil.cpp
  src/Util/StringUtil.cpp

  src/Application.cpp
  src/FreeCameraController.cpp
  src/SceneCache.cpp
)
add_library(edbr::edbr ALIAS edbr)

set_target_properties(edbr PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
)

target_add_extra_warnings(edbr)

target_include_directories(edbr PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

target_link_libraries(edbr
  PUBLIC
    volk::volk_headers
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator

    fmt::fmt
    glm::glm
    nlohmann_json::nlohmann_json
    utf8cpp::utf8cpp
    EnTT::EnTT
    imgui::imgui
    im3d::im3d

  PRIVATE
    tinygltf::tinygltf
    stb::image
    freetype::freetype
)

target_compile_definitions(edbr
  PUBLIC
    VK_NO_PROTOTYPES
    VMA_VULKAN_VERSION=1003000
)

# SDL
if(BUILD_SHARED_LIBS)
  target_link_libraries(edbr PUBLIC
    SDL2::SDL2
  )
else()
  target_link_libraries(edbr PUBLIC
    SDL2::SDL2-static
  )
endif()

if(WIN32)
  target_link_libraries(edbr PRIVATE SDL2::SDL2main)
endif()

# glm
target_compile_definitions(edbr
  PUBLIC
    GLM_FORCE_CTOR_INIT
    GLM_FORCE_XYZW_ONLY
    GLM_FORCE_EXPLICIT_CTOR
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
)

target_link_libraries(edbr PUBLIC Tracy::TracyClient)

if (MSVC)
  target_compile_definitions(TracyClient PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:TRACY_ENABLE>)
  target_compile_definitions(edbr PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:TRACY_ENABLE>)
else()
  if(NOT CMAKE_BUILD_TYPE OR
    CMAKE_BUILD_TYPE STREQUAL "Debug" OR
    CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(TracyClient PRIVATE TRACY_ENABLE)
    target_compile_definitions(edbr PRIVATE TRACY_ENABLE)
  endif()
endif()
