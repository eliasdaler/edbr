add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")
add_library(stb::image ALIAS stb_image)
