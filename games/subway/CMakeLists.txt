cmake_minimum_required(VERSION 3.21)

project(
	subway_game
	VERSION 0.1.0
	LANGUAGES CXX C
)

if (PROJECT_IS_TOP_LEVEL)
  set(EDBR_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
  add_subdirectory("${EDBR_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/edbr")
endif()

add_executable(subway_game
  src/Game.cpp

  src/main.cpp
)

set_target_properties(subway_game PROPERTIES
  CXX_STANDARD 20
  CXX_EXTENSIONS OFF
)

target_add_extra_warnings(subway_game)

target_include_directories(subway_game PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")

target_link_libraries(subway_game PRIVATE
  edbr::edbr
)

# Assets
symlink_assets(subway_game)
generate_spritesheets(subway_game)

# Build shaders
get_edbr_common_2d_shaders(EDBR_2D_SHADERS)
target_shaders(subway_game "${EDBR_2D_SHADERS}")

set_exe_target_common_props(subway_game)

# include("${CMAKE_CURRENT_LIST_DIR}/PlatformerInstall.cmake")

# Testing
include(CTest)
if(EDBR_BUILD_TESTING)
  enable_testing()
  # add_subdirectory(test)
endif()
