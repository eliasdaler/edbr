#include <cassert>

#include <array>
#include <cstdint>
#include <iostream>
#include <set>
#include <span>
#include <string_view>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#define VK_CHECK(call)                 \
    do {                               \
        VkResult result_ = call;       \
        assert(result_ == VK_SUCCESS); \
    } while (0)

bool checkValidationLayerSupport(std::string_view validationLayerName)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    for (const auto& layer : availableLayers) {
        if (std::string_view{layer.layerName} == validationLayerName) {
            return true;
        }
    }
    return false;
}

bool checkPhysicalDeviceSupportsNeededExtensions(VkPhysicalDevice device)
{
    auto deviceExtensions = std::array{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(
        device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}

VkPhysicalDevice pickPhysicalDevice(std::span<VkPhysicalDevice> physicalDevices)
{
    VkPhysicalDevice discreteGPU{};
    VkPhysicalDevice integratedGPU{};

    for (const auto& device : physicalDevices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            discreteGPU = device;
        }
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            integratedGPU = device;
        }
    }

    if (discreteGPU) {
        return discreteGPU;
    }

    if (integratedGPU) {
        return integratedGPU;
    }

    if (!physicalDevices.empty()) {
        return physicalDevices[0];
    }

    std::cout << "No physical devices found!\n";
    return 0;
}

VkInstance createVkInstance()
{
    const auto appInfo = VkApplicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_3,
    };
    auto createInfo = VkInstanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
    };

#ifndef NDEBUG
    static const char* KHRONOS_VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";
    assert(checkValidationLayerSupport(KHRONOS_VALIDATION_LAYER_NAME));
    const auto validationLayers = std::array{KHRONOS_VALIDATION_LAYER_NAME};
    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();
#endif

    std::vector<const char*> extensionNames{};
    { // add extensions required to create a surface from GLFW
        uint32_t count;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);
        assert(extensions);
        for (auto i = 0u; i < count; ++i) {
            extensionNames.push_back(extensions[i]);
        }
    }

    createInfo.ppEnabledExtensionNames = extensionNames.data();
    createInfo.enabledExtensionCount = extensionNames.size();

    VkInstance instance{};
    VK_CHECK(vkCreateInstance(&createInfo, 0, &instance));
    return instance;
}

void printDeviceInfo(const std::vector<VkPhysicalDevice>& physicalDevices)
{
    static const auto deviceTypes = std::array{
        "VK_PHYSICAL_DEVICE_TYPE_OTHER",
        "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU",
        "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU",
        "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU",
        "VK_PHYSICAL_DEVICE_TYPE_CPU",
    };
    for (std::uint32_t i = 0; i < physicalDevices.size(); ++i) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);
        std::cout << "deviceName: " << properties.deviceName << std::endl;
        std::cout << "deviceType: " << deviceTypes[properties.deviceType] << std::endl;
    }
}

VkPhysicalDevice createVkPhysicalDevice(VkInstance instance)
{
    std::uint32_t physicalDeviceCount{};
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));

    const auto physicalDevice = pickPhysicalDevice(physicalDevices);
    assert(physicalDevice);
    { // print name
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        std::cout << "Picked GPU: " << properties.deviceName << std::endl;
    }
    return physicalDevice;
}

std::uint32_t pickQueue(VkInstance instance, VkPhysicalDevice physicalDevice)
{
    std::uint32_t pickedQueue;

    std::uint32_t queueFamilyPropertiesCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, 0);

    std::vector<VkQueueFamilyProperties> familyProperties(queueFamilyPropertiesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice, &queueFamilyPropertiesCount, familyProperties.data());
    assert(!familyProperties.empty());

    bool queuePicked = false;
    for (std::uint32_t i = 0; i < familyProperties.size(); ++i) {
        const auto& props = familyProperties[i];
        if ((props.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, i)) {
            pickedQueue = i;
            queuePicked = true;
            break;
        }
    }
    assert(queuePicked);

    return pickedQueue;
}

VkDevice createVkDevice(
    VkInstance instance,
    VkPhysicalDevice physicalDevice,
    std::uint32_t* queueIndex)
{
    *queueIndex = pickQueue(instance, physicalDevice);
    float queuePriorities{1.f};
    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = *queueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriorities,
    };

    const auto deviceExtensions = std::array{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = deviceExtensions.size(),
        .ppEnabledExtensionNames = deviceExtensions.data(),
    };

    VkDevice device{};
    VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, 0, &device));
    return device;
}

bool checkSurfaceFormatSupported(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    VkSurfaceFormatKHR requiredFormat)
{
    std::uint32_t formatCount;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr));

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    VK_CHECK(
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, surfaceFormats.data()));
    assert(!surfaceFormats.empty());

    for (const auto& format : surfaceFormats) {
        if ((format.format == requiredFormat.format &&
             format.colorSpace == requiredFormat.colorSpace) ||
            format.format == VK_FORMAT_UNDEFINED) { // no preferred format
            return true;
        }
    }
    return false;
}

bool checkPresentModeSupported(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    VkPresentModeKHR requiredPresentMode)
{
    std::uint32_t presentModeCount;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, 0));

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeCount, presentModes.data()));
    for (const auto& presentMode : presentModes) {
        if (presentMode == requiredPresentMode) {
            return true;
        }
    }
    return false;
}

struct SwapchainInfo {
    VkSurfaceFormatKHR format;
    VkPresentModeKHR presentMode;
};
SwapchainInfo selectSwapchainProps(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapchainInfo info{
        .format =
            VkSurfaceFormatKHR{
                .format = VK_FORMAT_B8G8R8A8_UNORM,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            },
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    };

    assert(checkSurfaceFormatSupported(device, surface, info.format));
    assert(checkPresentModeSupported(device, surface, info.presentMode));

    return info;
}

VkSwapchainKHR createSwapchain(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkSurfaceKHR surface,
    std::uint32_t queueIndex,
    GLFWwindow* window)
{
    const auto info = selectSwapchainProps(physicalDevice, surface);

    VkSurfaceCapabilitiesKHR surfCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfCapabilities));

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    assert(surfCapabilities.maxImageExtent.width == (uint32_t)width);
    assert(surfCapabilities.maxImageExtent.height == (uint32_t)height);
    // FIXME: handle other cases later

    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surfCapabilities.minImageCount,
        .imageFormat = info.format.format,
        .imageColorSpace = info.format.colorSpace,
        .imageExtent = VkExtent2D{.width = (uint32_t)width, .height = (uint32_t)height},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queueIndex,
        .preTransform = surfCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = info.presentMode,
    };

    VkSwapchainKHR swapchain{};
    VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, 0, &swapchain));
    return swapchain;
}

int main()
{
    const auto rc = glfwInit();
    assert(rc);
    assert(glfwVulkanSupported());

    const auto instance = createVkInstance();
    assert(instance);
    const auto physicalDevice = createVkPhysicalDevice(instance);
    assert(physicalDevice);
    assert(checkPhysicalDeviceSupportsNeededExtensions(physicalDevice));
    std::uint32_t queueIndex{};
    const auto device = createVkDevice(instance, physicalDevice, &queueIndex);
    assert(device);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(1024, 768, "Vulkan app", nullptr, nullptr);
    assert(window);

    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, window, NULL, &surface));
    assert(surface);

    auto swapchain = createSwapchain(physicalDevice, device, surface, queueIndex, window);
    assert(swapchain);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    vkDestroySwapchainKHR(device, swapchain, 0);
    vkDestroySurfaceKHR(instance, surface, 0);
    vkDestroyDevice(device, 0);
    vkDestroyInstance(instance, 0);

    glfwDestroyWindow(window);
    glfwTerminate();
}
