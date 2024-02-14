#include <cassert>
#include <cstdio>

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

int main()
{
    assert(VK_FALSE == 0);
    printf("Hello, world\n");

    const auto rc = glfwInit();
    assert(rc);

    glfwWindowHint(GLFW_RESIZABLE, 0);
    auto window = glfwCreateWindow(1024, 768, "Vulkan app", nullptr, nullptr);
    assert(window);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
}
