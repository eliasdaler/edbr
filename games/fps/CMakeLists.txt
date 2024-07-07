cmake_minimum_required(VERSION 3.21)

project(
	fps
	VERSION 0.1.0
	LANGUAGES CXX C
)

if (PROJECT_IS_TOP_LEVEL)
  set(EDBR_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
  add_subdirectory("${EDBR_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/edbr")
endif()

add_executable(fps
  src/Game.cpp

  src/main.cpp
)

set_target_properties(fps PROPERTIES
  CXX_STANDARD 20
  CXX_EXTENSIONS OFF
)

target_add_extra_warnings(fps)

target_include_directories(fps PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")

target_link_libraries(fps PRIVATE
  edbr::edbr
)

# Assets
symlink_assets(fps)
generate_spritesheets(fps)

# Build shaders
get_edbr_common_2d_shaders(EDBR_2D_SHADERS)
target_shaders(fps "${EDBR_2D_SHADERS}")

set_exe_target_common_props(fps)

# include("${CMAKE_CURRENT_LIST_DIR}/FPSInstall.cmake")

# Testing
include(CTest)
if(EDBR_BUILD_TESTING)
  enable_testing()
  # add_subdirectory(test)
endif()
