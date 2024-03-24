add_executable(mtpgame
  src/DevTools/CustomEntityTreeView.cpp

  src/EntityInitializer.cpp
  src/EntityUtil.cpp
  src/Game.cpp
  src/GameComponentDisplayers.cpp
  src/GameDevTools.cpp
  src/GameSceneLoader.cpp
  src/GameUI.cpp
  src/Level.cpp

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

# copy assets
add_custom_command(TARGET mtpgame POST_BUILD
  COMMENT "Copying mtp assets to $<TARGET_FILE_DIR:mtpgame>/assets"
  COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:mtpgame>/assets"
)

# build shaders
set(SHADERS
  fullscreen_triangle.vert
  skybox.frag
  skinning.comp
  mesh.vert
  mesh_depth_only.vert
  mesh.frag
  postfx.frag
  depth_resolve.frag
  sprite.vert
  sprite.frag
  imgui.vert
  imgui.frag
)

get_target_property(EDBR_SOURCE_DIR edbr SOURCE_DIR)
set(EDBR_SHADERS_DIR "${EDBR_SOURCE_DIR}/src/shaders/")
list(TRANSFORM SHADERS PREPEND "${EDBR_SHADERS_DIR}")

target_shaders(edbr ${SHADERS})

