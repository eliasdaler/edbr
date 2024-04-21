cmake_minimum_required(VERSION 3.21)

project(
  EDBR
  VERSION 0.1.0
  LANGUAGES CXX C
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ColorDiagnostics)
include(TargetAddExtraWarnings)
include(TargetShaders)
include(CopyDepsToRuntimeDir)

if (WIN32)	
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Link to libs dynamically")
else()
  set(BUILD_SHARED_LIBS ON CACHE BOOL "Link to libs dynamically")
endif()

if (MSVC)
  # Disable Just My Code debugging
  add_compile_options(/JMC-)
endif()

# Check that git submodules were cloned
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/edbr/third_party/SDL/CMakeLists.txt")
  include(FetchSubmodules)
  fetch_submodules()
endif()

option(EDBR_BUILD_TESTING "Build tests" OFF)

add_subdirectory(edbr)

if (PROJECT_IS_TOP_LEVEL)
	add_subdirectory(games)
endif()
