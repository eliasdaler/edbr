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
  src/EntityUtil.cpp
  src/GameComponents.cpp
  src/GameComponentDisplayers.cpp
  src/GameDevTools.cpp
  src/GameEntityInit.cpp
  src/GameUI.cpp
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

# Assets
symlink_assets(platformer)
generate_spritesheets(platformer)

# Build shaders
get_edbr_common_2d_shaders(EDBR_2D_SHADERS)
target_shaders(platformer "${EDBR_2D_SHADERS}")

set_exe_target_common_props(platformer)

include("${CMAKE_CURRENT_LIST_DIR}/PlatformerInstall.cmake")

# Testing
include(CTest)
if(EDBR_BUILD_TESTING)
  enable_testing()
  # add_subdirectory(test)
endif()
