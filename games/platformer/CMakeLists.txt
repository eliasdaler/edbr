cmake_minimum_required(VERSION 3.21)

project(
	platformer
	VERSION 0.1.0
	LANGUAGES CXX C
)

if (PROJECT_IS_TOP_LEVEL)
  set(EDBR_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
  add_subdirectory("${EDBR_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/edbr")
endif()

add_executable(platformer
  src/Level.cpp
  src/Game.cpp

  src/main.cpp
)

set_target_properties(platformer PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
)

target_add_extra_warnings(platformer)

target_include_directories(platformer PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")

target_link_libraries(platformer PRIVATE
  edbr::edbr
)

# Symlink assets.
# Symlinking (instead of copying) allows you to change resources and have the
# changes be instantly visible when reloading the game
add_custom_command(TARGET platformer POST_BUILD
  COMMENT "Copying mtp assets to $<TARGET_FILE_DIR:platformer>/assets"
  COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:platformer>/assets"
)

# build shaders
set(SHADERS
  fullscreen_triangle.vert
  sprite.vert
  sprite.frag

  imgui.vert
  imgui.frag
)

get_target_property(EDBR_SOURCE_DIR edbr SOURCE_DIR)
set(EDBR_SHADERS_DIR "${EDBR_SOURCE_DIR}/src/shaders/")
list(TRANSFORM SHADERS PREPEND "${EDBR_SHADERS_DIR}")

target_shaders(platformer ${SHADERS})

include(CTest)
if(EDBR_BUILD_TESTING)
  enable_testing()
  # add_subdirectory(test)
endif()

if(WIN32)
  set_target_properties(platformer
    PROPERTIES
      LINK_FLAGS_DEBUG   "/SUBSYSTEM:CONSOLE"
      LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
  )
  copy_deps_to_runtime_dir(platformer)
endif()

set_target_properties(platformer PROPERTIES
  INSTALL_RPATH $ORIGIN/lib
)

include("${CMAKE_CURRENT_LIST_DIR}/PlatformerInstall.cmake")
