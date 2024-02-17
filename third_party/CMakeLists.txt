add_subdirectory(glfw)
add_library(glfw::glfw ALIAS glfw)

add_subdirectory(vk-bootstrap)
add_subdirectory(vma)
