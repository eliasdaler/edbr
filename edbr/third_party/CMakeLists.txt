# glm
add_subdirectory(glm)

# stb
add_subdirectory(stb)

# SDL
if (NOT BUILD_SHARED_LIBS)
  set(SDL_SHARED_ENABLED_BY_DEFAULT OFF CACHE BOOL "Don't build SDL as shared lib")
endif()
option(SDL_TEST "Build the SDL2_test library" OFF)
option(SDL_AUDIO_ENABLED_BY_DEFAULT "Enable the Audio subsystem" OFF)
add_subdirectory(SDL)

# vk-bootstrap
add_subdirectory(vk-bootstrap)
if (MSVC)
  target_compile_definitions(vk-bootstrap PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
endif()
if (BUILD_SHARED_LIBS)
  set_target_properties(vk-bootstrap PROPERTIES
    POSITION_INDEPENDENT_CODE ON
  )
endif()

# vma
add_subdirectory(vma)
# volk
add_subdirectory(volk)

## Dear ImGui
set(IMGUI_SOURCES
  "imgui.cpp"
  "imgui_draw.cpp"
  "imgui_tables.cpp"
  "imgui_widgets.cpp"
  "imgui_demo.cpp"
  # C++ additions
  "misc/cpp/imgui_stdlib.cpp"
  # backend
  "backends/imgui_impl_sdl2.cpp"
)

list(TRANSFORM IMGUI_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/imgui/")

add_library(imgui STATIC ${IMGUI_SOURCES})
set_property(TARGET imgui PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(imgui
  PUBLIC
    "imgui"
    "imgui/backends"
)

if(BUILD_SHARED_LIBS)
  target_link_libraries(imgui
    PUBLIC
      SDL2::SDL2
  )
else()
  target_link_libraries(imgui
    PUBLIC
      SDL2::SDL2-static
  )
endif()

add_library(imgui::imgui ALIAS imgui)

## im3d
add_library(im3d
  "${CMAKE_CURRENT_LIST_DIR}/im3d/im3d.cpp"
)
add_library(im3d::im3d ALIAS im3d)
if (MSVC)
  target_compile_definitions(im3d PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
endif()

target_include_directories(im3d PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}/im3d"
)

## tinygltf
option(TINYGLTF_BUILD_LOADER_EXAMPLE "Build loader_example(load glTF and dump infos)" OFF)
option(TINYGLTF_INSTALL "Install tinygltf files during install step." OFF)
option(TINYGLTF_HEADER_ONLY "On: header-only mode. Off: create tinygltf library(No TINYGLTF_IMPLEMENTATION required in your project)" ON)
add_subdirectory(tinygltf)
add_library(tinygltf::tinygltf ALIAS tinygltf)

## tracy
option(TRACY_ENABLE "" OFF)
option(TRACY_ON_DEMAND "" OFF)
add_subdirectory(tracy)
if (MSVC)
  target_compile_definitions(TracyClient PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
endif()

## freetype
option(FT_DISABLE_ZLIB
  "Disable use of system zlib and use internal zlib library instead." ON)
option(FT_DISABLE_BZIP2
  "Disable support of bzip2 compressed fonts." ON)
option(FT_DISABLE_BROTLI
  "Disable support of compressed WOFF2 fonts." ON)
option(FT_DISABLE_HARFBUZZ
  "Disable HarfBuzz (used for improving auto-hinting of OpenType fonts)." ON)
option(FT_DISABLE_PNG
  "Disable support of PNG compressed OpenType embedded bitmaps." ON)
option(SKIP_INSTALL_ALL "Skip install all" ON)
option(FT_ENABLE_ERROR_STRINGS
  "Enable support for meaningful error descriptions." ON)
add_subdirectory(freetype)
add_library(freetype::freetype ALIAS freetype)

## utfcpp
add_subdirectory(utfcpp)
add_library(utf8cpp::utf8cpp ALIAS utf8cpp)

## entt
add_subdirectory(entt)

## json
option(JSON_MultipleHeaders "Use non-amalgamated version of the library." ON)
option(JSON_Install         "Install CMake targets during install step." OFF)
add_subdirectory(json)

## fmt
add_subdirectory(fmt)
set_target_properties(fmt PROPERTIES
  PUBLIC_HEADER ""
)
if (MSVC)
  target_compile_definitions(fmt PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
endif()

## jolt
if(NOT CMAKE_BUILD_TYPE OR
  CMAKE_BUILD_TYPE STREQUAL "Debug" OR
  CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  option(GENERATE_DEBUG_SYMBOLS "Generate debug symbols" ON)
else()
  option(GENERATE_DEBUG_SYMBOLS "Generate debug symbols" OFF)
endif()
option(OVERRIDE_CXX_FLAGS "Override CMAKE_CXX_FLAGS_DEBUG/RELEASE" OFF)
option(ENABLE_ALL_WARNINGS "Enable all warnings and warnings as errors" OFF)
option(USE_STATIC_MSVC_RUNTIME_LIBRARY "Use the static MSVC runtime library" OFF)
option(DEBUG_RENDERER_IN_DISTRIBUTION "Enable debug renderer in Distribution builds" ON)
add_subdirectory(JoltPhysics/Build)
add_library(Jolt::Jolt ALIAS Jolt)
if (MSVC)
  target_compile_definitions(Jolt PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
endif()

## openal-soft

# Disable PipeWire because if you build on systems where pipewire is installed,
# then it won't work on systems without PipeWire
option(ALSOFT_BACKEND_PIPEWIRE "Enable PipeWire backend" OFF)

# disabled SNDIO, most Linuxes should have ALSA or PulseAudio
option(ALSOFT_BACKEND_SNDIO "Enable SndIO backend" OFF)

option(ALSOFT_UTILS "Build utility programs"  OFF)
option(ALSOFT_EXAMPLES  "Build example programs"  OFF)
option(ALSOFT_INSTALL "Install main library" OFF)
option(ALSOFT_INSTALL_CONFIG "Install alsoft.conf sample configuration file" OFF)
option(ALSOFT_INSTALL_HRTF_DATA "Install HRTF data files" OFF)
option(ALSOFT_INSTALL_AMBDEC_PRESETS "Install AmbDec preset files" OFF)
option(ALSOFT_INSTALL_EXAMPLES "Install example programs (alplay, alstream, ...)" OFF)
option(ALSOFT_INSTALL_UTILS "Install utility programs (openal-info, alsoft-config, ...)" OFF)
option(ALSOFT_UPDATE_BUILD_VERSION "Update git build version info" OFF)

add_subdirectory(openal-soft)
if (MSVC)
  target_compile_definitions(alcommon PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
  target_compile_definitions(OpenAL PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
endif()

# mute some annoying warnings
include(OpenALSoftFlags)
open_al_soft_flags(alcommon)
open_al_soft_flags(OpenAL)

## ogg
option(INSTALL_DOCS "Install documentation" OFF)
option(INSTALL_PKG_CONFIG_MODULE "Install ogg.pc file" OFF)
option(INSTALL_CMAKE_PACKAGE_MODULE "Install CMake package configuration module" OFF)
add_subdirectory(ogg)

set_target_properties(ogg PROPERTIES
  PUBLIC_HEADER ""
)

## libvorbis
option(INSTALL_CMAKE_PACKAGE_MODULE "Install CMake package configiguration module" OFF)
add_subdirectory(vorbis)
add_library(vorbis::vorbis ALIAS vorbis)
add_library(vorbis::vorbisfile ALIAS vorbisfile)

## Google test
if(EDBR_BUILD_TESTING)
  if (MSVC AND NOT USE_STATIC_RUNTIME)
    set(gtest_force_shared_crt ON CACHE BOOL "Use shared CRT")
  endif()

  add_subdirectory(googletest)
  if (MSVC)
    target_compile_definitions(gtest PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
    target_compile_definitions(gtest_main PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
    target_compile_definitions(gmock PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
  endif()
endif()

## CLI11
add_subdirectory(CLI11)

# cute_headers (Aseprite support)
add_subdirectory(cute_headers)
