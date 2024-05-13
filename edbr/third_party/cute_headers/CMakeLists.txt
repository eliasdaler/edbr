add_library(cute_headers_aseprite INTERFACE)
target_include_directories(cute_headers_aseprite INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")
add_library(cute_headers::aseprite ALIAS cute_headers_aseprite)
