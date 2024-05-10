add_subdirectory(third_party)

add_library(edbr
  # Core
  src/Core/JsonDataLoader.cpp
  src/Core/JsonFile.cpp
  src/Core/JsonMath.cpp
  src/Core/JsonGraphics.cpp

  # DevTools
  src/DevTools/ActionListInspector.cpp
  src/DevTools/EntityInfoDisplayer.cpp
  src/DevTools/EntityTreeView.cpp
  src/DevTools/Im3dState.cpp
  src/DevTools/ImGuiPropertyTable.cpp
  src/DevTools/JoltDebugRenderer.cpp
  src/DevTools/ResourcesInspector.cpp
  src/DevTools/UIInspector.cpp

  # Event
  src/Event/Event.cpp
  src/Event/EventManager.cpp
  src/Event/ListenerInfo.cpp

  # ECS
  src/ECS/ComponentFactory.cpp
  src/ECS/EntityFactory.cpp
  src/ECS/Systems/MovementSystem.cpp
  src/ECS/Systems/TransformSystem.cpp

  # Math
  src/Math/IndexRange2.cpp
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
  src/Graphics/Bouncer.cpp
  src/Graphics/Camera.cpp
  src/Graphics/Color.cpp
  src/Graphics/Cubemap.cpp
  src/Graphics/Font.cpp
  src/Graphics/FrustumCulling.cpp
  src/Graphics/GfxDevice.cpp
  src/Graphics/ImageCache.cpp
  src/Graphics/ImageLoader.cpp
  src/Graphics/Letterbox.cpp
  src/Graphics/MaterialCache.cpp
  src/Graphics/MeshCache.cpp
  src/Graphics/MipMapGeneration.cpp
  src/Graphics/NBuffer.cpp
  src/Graphics/Scene.cpp
  src/Graphics/ShadowMapping.cpp
  src/Graphics/SkeletonAnimator.cpp
  src/Graphics/SkeletalAnimation.cpp
  src/Graphics/SkeletalAnimationCache.cpp
  src/Graphics/Sprite.cpp
  src/Graphics/SpriteAnimator.cpp
  src/Graphics/SpriteAnimationData.cpp

  # Graphics/Pipeline
  src/Graphics/Pipelines/CSMPipeline.cpp
  src/Graphics/Pipelines/DepthResolvePipeline.cpp
  src/Graphics/Pipelines/MeshPipeline.cpp
  src/Graphics/Pipelines/PostFXPipeline.cpp
  src/Graphics/Pipelines/SkinningPipeline.cpp
  src/Graphics/Pipelines/SkyboxPipeline.cpp
  src/Graphics/Pipelines/SpriteDrawingPipeline.cpp

  # Graphics - high level renderers
  src/Graphics/GameRenderer.cpp
  src/Graphics/SpriteRenderer.cpp

  # Text
  src/Text/TextManager.cpp

  # Util
  src/Util/CameraUtil.cpp
  src/Util/GltfLoader.cpp
  src/Util/Im3dUtil.cpp
  src/Util/ImGuiUtil.cpp
  src/Util/InputUtil.cpp
  src/Util/MetaUtil.cpp
  src/Util/OSUtil.cpp
  src/Util/Palette.cpp
  src/Util/StringUtil.cpp

  src/Input/ActionMapping.cpp
  src/Input/ButtonState.cpp
  src/Input/GamepadState.cpp
  src/Input/InputManager.cpp
  src/Input/InputStringMap.cpp
  src/Input/KeyboardState.cpp
  src/Input/MouseState.cpp

  # Audio
  src/Audio/AudioManager.cpp
  src/Audio/Music.cpp
  src/Audio/OpenALUtil.cpp
  src/Audio/Sound.cpp
  src/Audio/SoundBuffer.cpp
  src/Audio/SoundDevice.cpp
  src/Audio/WavLoader.cpp

  # UI
  src/UI/ButtonElement.cpp
  src/UI/Element.cpp
  src/UI/ImageElement.cpp
  src/UI/InputEvent.cpp
  src/UI/ListLayoutElement.cpp
  src/UI/NineSlice.cpp
  src/UI/NineSliceElement.cpp
  src/UI/Style.cpp
  src/UI/TextElement.cpp
  src/UI/UIRenderer.cpp

  # Game UI
  src/GameUI/Cursor.cpp
  src/GameUI/DialogueBox.cpp
  src/GameUI/MenuStack.cpp

  # ActionList
  src/ActionList/ActionList.cpp
  src/ActionList/ActionListManager.cpp
  src/ActionList/ActionWrappers.cpp
  # ActionList - default actions
  src/ActionList/Actions/ActionListFinishAction.cpp
  src/ActionList/Actions/DelayAction.cpp
  src/ActionList/Actions/DoAction.cpp
  src/ActionList/Actions/MakeAction.cpp
  src/ActionList/Actions/ParallelActionListsFinishAction.cpp
  src/ActionList/Actions/TweenAction.cpp
  src/ActionList/Actions/WaitWhileAction.cpp

  # Camera
  src/Camera/FreeCameraController.cpp
  src/Camera/CameraActions.cpp
  src/Camera/CameraManager.cpp

  # Save
  src/Save/SaveFile.cpp
  src/Save/SaveFileManager.cpp

  # TileMap
  src/TileMap/TileMap.cpp
  src/TileMap/TileMapRenderer.cpp

  # GameCommon
  src/GameCommon/CommonComponentDisplayers.cpp
  src/GameCommon/CommonComponentLoaders.cpp
  src/GameCommon/CommonComponentLoaders2D.cpp
  src/GameCommon/DialogueActions.cpp
  src/GameCommon/EntityUtil2D.cpp
  src/GameCommon/EntityUtil.cpp

  src/Application.cpp
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

    Jolt::Jolt

    # audio
    OpenAL::OpenAL
    Ogg::ogg
    vorbis::vorbis
    vorbis::vorbisfile

    CLI11::CLI11
  PRIVATE
    tinygltf::tinygltf
    stb::image
    freetype::freetype
)

target_compile_definitions(edbr PUBLIC AL_ALEXT_PROTOTYPES)

target_compile_definitions(edbr
  PUBLIC
    VK_NO_PROTOTYPES
    VMA_VULKAN_VERSION=1003000
    # VOLK_DEFAULT_VISIBILITY # FIXME: doesn't work for some reason
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

target_compile_definitions(edbr
  PUBLIC
    JPH_DEBUG_RENDERER
)

target_link_libraries(edbr PUBLIC Tracy::TracyClient)
if (MSVC)
  target_compile_definitions(TracyClient PUBLIC
    $<$<CONFIG:Debug,RelWithDebInfo>:TRACY_ENABLE>
    $<$<CONFIG:Debug,RelWithDebInfo>:TRACY_DELAYED_INIT> # for unit tests to work
  )
else()
  if(NOT CMAKE_BUILD_TYPE OR
    CMAKE_BUILD_TYPE STREQUAL "Debug" OR
    CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(TracyClient PUBLIC
        TRACY_ENABLE
        TRACY_DELAYED_INIT # needed for unit tests to work
    )
  endif()
endif()

## testing
include(CTest)
if(EDBR_BUILD_TESTING)
  enable_testing()
  if(NOT MSVC)
	add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)
  else()
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -C Debug --output-on-failure)
  endif()
  add_subdirectory(test)
endif()
