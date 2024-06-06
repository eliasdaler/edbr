cmake_minimum_required(VERSION 3.21)

project(
	mtpgame
	VERSION 0.1.0
	LANGUAGES CXX C
)

if (PROJECT_IS_TOP_LEVEL)
  set(EDBR_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
  add_subdirectory("${EDBR_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/edbr")
endif()

add_executable(mtpgame
  src/DevTools/CustomEntityTreeView.cpp

  src/AnimationSoundSystem.cpp
  src/EntityCreator.cpp
  src/EntityUtil.cpp
  src/FollowCameraController.cpp
  src/Level.cpp
  src/PhysicsSystem.cpp
  src/VirtualCharacterParams.cpp

  src/Game.cpp
  src/GameComponents.cpp
  src/GameComponentDisplayers.cpp
  src/GameEntityInit.cpp
  src/GameDevTools.cpp
  src/GameUI.cpp
  src/GameLevels.cpp

  src/MTPSaveFile.cpp

  src/main.cpp
)

set_target_properties(mtpgame PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
)

target_add_extra_warnings(mtpgame)

target_include_directories(mtpgame PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")

target_link_libraries(mtpgame PRIVATE
  edbr::edbr
)

# Symlink assets.
# Symlinking (instead of copying) allows you to change resources and have the
# changes be instantly visible when reloading the game
add_custom_command(TARGET mtpgame POST_BUILD
  COMMENT "Copying mtp assets to $<TARGET_FILE_DIR:mtpgame>/assets"
  COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:mtpgame>/assets"
)

# build shaders
set(SHADERS
  fullscreen_triangle.vert
  skybox.frag
  skinning.comp
  mesh.vert
  mesh_depth_only.vert
  mesh_depth.frag
  mesh.frag
  postfx.frag
  depth_resolve.frag
  sprite.vert
  sprite.frag

  imgui.vert
  imgui.frag

  im3d/im3d_points.vert
  im3d/im3d_points.frag

  im3d/im3d_lines.vert
  im3d/im3d_lines.geom
  im3d/im3d_lines.frag

  im3d/im3d_triangles.vert
  im3d/im3d_triangles.frag

)

get_target_property(EDBR_SOURCE_DIR edbr SOURCE_DIR)
set(EDBR_SHADERS_DIR "${EDBR_SOURCE_DIR}/src/shaders/")
list(TRANSFORM SHADERS PREPEND "${EDBR_SHADERS_DIR}")

target_shaders(mtpgame ${SHADERS})

include(CTest)
if(EDBR_BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()

if(WIN32)
  set_target_properties(mtpgame
    PROPERTIES
      LINK_FLAGS_DEBUG   "/SUBSYSTEM:CONSOLE"
      LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
  )
  copy_deps_to_runtime_dir(mtpgame)
endif()

set_target_properties(mtpgame PROPERTIES
  INSTALL_RPATH $ORIGIN/lib
)

include("${CMAKE_CURRENT_LIST_DIR}/MTPInstall.cmake")
