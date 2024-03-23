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

# vkb
add_subdirectory(vk-bootstrap)
add_library(vkb
  "${CMAKE_CURRENT_SOURCE_DIR}/vk-bootstrap/src/VkBootstrap.cpp"
)
target_include_directories(vkb PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/vk-bootstrap/src/"
)
add_library(vkb::vkb ALIAS vkb)

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
  "backends/imgui_impl_vulkan.cpp"
  "backends/imgui_impl_sdl2.cpp"
)

list(TRANSFORM IMGUI_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/imgui/")

add_library(imgui STATIC ${IMGUI_SOURCES})
set_property(TARGET imgui PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(imgui
  PRIVATE
    IMGUI_IMPL_VULKAN_NO_PROTOTYPES
)

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

## tinygltf
option(TINYGLTF_BUILD_LOADER_EXAMPLE "Build loader_example(load glTF and dump infos)" OFF)
option(TINYGLTF_INSTALL "Install tinygltf files during install step." OFF)
option(TINYGLTF_HEADER_ONLY "On: header-only mode. Off: create tinygltf library(No TINYGLTF_IMPLEMENTATION required in your project)" ON)
add_subdirectory(tinygltf)
add_library(tinygltf::tinygltf ALIAS tinygltf)

## tracy
option(TRACY_ENABLE "" OFF)
option(TRACY_ON_DEMAND "" ON)
add_subdirectory(tracy)

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

