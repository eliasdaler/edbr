add_executable(game
  main.cpp
)

set_target_properties(game PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
)

target_add_extra_warnings(game)

target_include_directories(game PRIVATE "${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(game PRIVATE glfw::glfw)

find_package(Vulkan REQUIRED)
target_link_libraries(game PRIVATE Vulkan::Vulkan)
