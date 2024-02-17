#include <cassert>
#include <cstdint>

#include <vulkan/vulkan.h>

#include <VkBootstrap.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <Renderer.h>
#include <VkUtil.h>

int main()
{
    static constexpr std::uint32_t SCREEN_WIDTH = 1024;
    static constexpr std::uint32_t SCREEN_HEIGHT = 768;

    vkb::InstanceBuilder builder;
    auto instance = builder.set_app_name("Vulkan app")
                        .request_validation_layers()
                        .use_default_debug_messenger()
                        .require_api_version(1, 3, 0)
                        .build()
                        .value();

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Vulkan app", nullptr, nullptr);
    assert(window);

    vkb::PhysicalDeviceSelector selector{instance};

    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, window, 0, &surface));

    auto physicalDevice = selector.set_minimum_version(1, 3).set_surface(surface).select().value();

    vkb::DeviceBuilder device_builder{physicalDevice};
    VkPhysicalDeviceSynchronization2Features enabledFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .synchronization2 = true,
    };
    auto vkb_device = device_builder.add_pNext(&enabledFeatures).build().value();
    auto device = vkb_device.device;

    auto graphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    auto graphicsQueueFamily = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    vkb::SwapchainBuilder swapchainBuilder{vkb_device};
    auto swapchain = swapchainBuilder.use_default_format_selection()
                         .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                         .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                         .set_desired_extent(SCREEN_WIDTH, SCREEN_HEIGHT)
                         .build()
                         .value();

    auto swapchainImages = vkutil::getSwapchainImages(device, swapchain);

    Renderer renderer(graphicsQueue, graphicsQueueFamily);
    renderer.createCommandBuffers(device);
    renderer.initSyncStructures(device);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer.draw(device, swapchain, swapchainImages);
    }

    renderer.destroyCommandBuffers(device);
    renderer.destroySyncStructures(device);

    vkb::destroy_swapchain(swapchain);
    vkb::destroy_surface(instance, surface);
    vkb::destroy_device(vkb_device);
    vkb::destroy_instance(instance);

    glfwDestroyWindow(window);
    glfwTerminate();
}
