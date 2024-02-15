#include <cassert>

#include <array>
#include <cmath>
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

    VkPhysicalDeviceSynchronization2Features enabledFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .synchronization2 = true,
    };

    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabledFeatures,
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
static const SwapchainInfo swapchainInfo{
    .format =
        VkSurfaceFormatKHR{
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        },
    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
};

SwapchainInfo selectSwapchainProps(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    assert(checkSurfaceFormatSupported(device, surface, swapchainInfo.format));
    assert(checkPresentModeSupported(device, surface, swapchainInfo.presentMode));

    return swapchainInfo;
}

VkSwapchainKHR createSwapchain(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkSurfaceKHR surface,
    std::uint32_t queueIndex,
    GLFWwindow* window)
{
    VkSurfaceCapabilitiesKHR surfCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfCapabilities));

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    const auto swapchainExtent = VkExtent2D{
        .width = (uint32_t)width,
        .height = (uint32_t)height,
    };

    // FIXME: handle other cases later
    assert(surfCapabilities.maxImageExtent.width == swapchainExtent.width);
    assert(surfCapabilities.maxImageExtent.height == swapchainExtent.height);

    const auto compositeAlphaFlags = std::array{
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    auto compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    for (const auto flags : compositeAlphaFlags) {
        if (surfCapabilities.supportedCompositeAlpha & flags) {
            compositeAlpha = flags;
            break;
        }
    }

    const auto info = selectSwapchainProps(physicalDevice, surface);
    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surfCapabilities.minImageCount,
        .imageFormat = info.format.format,
        .imageColorSpace = info.format.colorSpace,
        .imageExtent = swapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queueIndex,
        .preTransform = surfCapabilities.currentTransform,
        .compositeAlpha = compositeAlpha,
        .presentMode = info.presentMode,
        .clipped = VK_TRUE,
    };

    VkSwapchainKHR swapchain{};
    VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, 0, &swapchain));
    return swapchain;
}

std::vector<VkImage> getSwapchainImages(VkDevice device, VkSwapchainKHR swapchain)
{
    std::uint32_t swapchainImageCount{};
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr));

    std::vector<VkImage> swapchainImages(swapchainImageCount);
    VK_CHECK(
        vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data()));
    return swapchainImages;
}

std::vector<VkImageView> createSwapchainImageViews(
    const std::vector<VkImage>& swapchainImages,
    VkDevice device,
    VkSwapchainKHR swapchain)
{
    std::vector<VkImageView> swapchainImageViews(swapchainImages.size());
    for (std::uint32_t i = 0; i < swapchainImages.size(); ++i) {
        const auto createInfo = VkImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchainInfo.format.format,
            .components =
                VkComponentMapping{
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };
        VK_CHECK(vkCreateImageView(device, &createInfo, 0, &swapchainImageViews[i]));
    }
    return swapchainImageViews;
}

struct FrameData {
    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;

    VkSemaphore swapchainSemaphore;
    VkSemaphore renderSemaphore;
    VkFence renderFence;
};

VkFenceCreateInfo createFence(VkFenceCreateFlags flags = 0)
{
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;

    info.flags = flags;

    return info;
}

VkImageSubresourceRange makeSubresourceRange(VkImageAspectFlags aspectMask)
{
    return VkImageSubresourceRange{
        .aspectMask = aspectMask,
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };
}

void transitionImage(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout currentLayout,
    VkImageLayout newLayout)
{
    VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ?
                                        VK_IMAGE_ASPECT_DEPTH_BIT :
                                        VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageMemoryBarrier2 imageBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
        .oldLayout = currentLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = makeSubresourceRange(aspectMask),
    };

    VkDependencyInfo depInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier,
    };

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore)
{
    return VkSemaphoreSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = semaphore,
        .value = 1,
        .stageMask = stageMask,
        .deviceIndex = 0,
    };
}

VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd)
{
    return VkCommandBufferSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cmd,
        .deviceMask = 0,
    };
}

VkSubmitInfo2 submitInfo(
    VkCommandBufferSubmitInfo* cmd,
    VkSemaphoreSubmitInfo* signalSemaphoreInfo,
    VkSemaphoreSubmitInfo* waitSemaphoreInfo)
{
    return VkSubmitInfo2{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = waitSemaphoreInfo ? 1u : 0u,
        .pWaitSemaphoreInfos = waitSemaphoreInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = cmd,
        .signalSemaphoreInfoCount = signalSemaphoreInfo ? 1u : 0u,
        .pSignalSemaphoreInfos = signalSemaphoreInfo,
    };
}

constexpr std::size_t FRAME_OVERLAP = 2;

class Renderer {
public:
    std::array<FrameData, FRAME_OVERLAP> frames{};
    std::uint32_t frameNumber{0};
    VkQueue graphicsQueue;
    std::uint32_t graphicsQueueFamily;

    FrameData& getCurrentFrame() { return frames[frameNumber % FRAME_OVERLAP]; }

    void createCommandBuffers(VkDevice device)
    {
        VkCommandPoolCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = graphicsQueueFamily,
        };
        for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
            auto& commandPool = frames[i].commandPool;
            VK_CHECK(vkCreateCommandPool(device, &createInfo, 0, &commandPool));
            VkCommandBufferAllocateInfo cmdAllocInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = commandPool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };

            auto& mainCommandBuffer = frames[i].mainCommandBuffer;
            VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &mainCommandBuffer));
        }
    }

    void initSyncStructures(VkDevice device)
    {
        const auto fenceCreateInfo = VkFenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        const auto semaphoreCreateInfo = VkSemaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
            VK_CHECK(vkCreateFence(device, &fenceCreateInfo, 0, &frames[i].renderFence));
            VK_CHECK(
                vkCreateSemaphore(device, &semaphoreCreateInfo, 0, &frames[i].swapchainSemaphore));
            VK_CHECK(
                vkCreateSemaphore(device, &semaphoreCreateInfo, 0, &frames[i].renderSemaphore));
        }
    }

    void destroyCommandBuffers(VkDevice device)
    {
        vkDeviceWaitIdle(device);

        for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
            vkDestroyCommandPool(device, frames[i].commandPool, 0);
        }
    }

    void destroySyncStructures(VkDevice device)
    {
        for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
            vkDestroyFence(device, frames[i].renderFence, 0);
            vkDestroySemaphore(device, frames[i].swapchainSemaphore, 0);
            vkDestroySemaphore(device, frames[i].renderSemaphore, 0);
        }
    }

    void draw(
        VkDevice device,
        VkSwapchainKHR swapchain,
        const std::vector<VkImage>& swapchainImages)
    {
        static constexpr auto defaultTimeout = std::numeric_limits<std::uint64_t>::max();

        const auto& currentFrame = getCurrentFrame();
        VK_CHECK(vkWaitForFences(device, 1, &currentFrame.renderFence, true, defaultTimeout));
        VK_CHECK(vkResetFences(device, 1, &currentFrame.renderFence));

        std::uint32_t swapchainImageIndex{};

        VK_CHECK(vkAcquireNextImageKHR(
            device,
            swapchain,
            defaultTimeout,
            currentFrame.swapchainSemaphore,
            0,
            &swapchainImageIndex));

        VkCommandBuffer cmd = currentFrame.mainCommandBuffer;
        VkCommandBufferBeginInfo cmdBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        auto& image = swapchainImages[swapchainImageIndex];
        transitionImage(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        { // clear
            auto flash = std::abs(std::sin(frameNumber / 120.f));
            auto clearValue = VkClearColorValue{{0.0f, 0.0f, flash, 1.0f}};
            auto clearRange = makeSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
            vkCmdClearColorImage(cmd, image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
        }

        transitionImage(cmd, image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        VK_CHECK(vkEndCommandBuffer(cmd));

        { // submit
            auto cmdinfo = commandBufferSubmitInfo(cmd);
            auto waitInfo = semaphoreSubmitInfo(
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                currentFrame.swapchainSemaphore);
            auto signalInfo = semaphoreSubmitInfo(
                VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, currentFrame.renderSemaphore);

            auto submit = submitInfo(&cmdinfo, &signalInfo, &waitInfo);
            VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, currentFrame.renderFence));
        }

        { // present
            auto presentInfo = VkPresentInfoKHR{
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &currentFrame.renderSemaphore,
                .swapchainCount = 1,
                .pSwapchains = &swapchain,
                .pImageIndices = &swapchainImageIndex,
            };

            VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));
        }

        // increase the number of frames drawn
        frameNumber++;
    }
};

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

    auto swapchainImages = getSwapchainImages(device, swapchain);
    auto swapchainImageViews = createSwapchainImageViews(swapchainImages, device, swapchain);
    assert(!swapchainImageViews.empty());

    VkQueue queue;
    vkGetDeviceQueue(device, queueIndex, 0, &queue);
    assert(queue);

    Renderer renderer{
        .graphicsQueue = queue,
        .graphicsQueueFamily = queueIndex,
    };
    renderer.createCommandBuffers(device);
    renderer.initSyncStructures(device);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer.draw(device, swapchain, swapchainImages);
    }

    for (const auto imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, 0);
    }

    renderer.destroyCommandBuffers(device);
    renderer.destroySyncStructures(device);

    vkDestroySwapchainKHR(device, swapchain, 0);
    vkDestroySurfaceKHR(instance, surface, 0);
    vkDestroyDevice(device, 0);
    vkDestroyInstance(instance, 0);

    glfwDestroyWindow(window);
    glfwTerminate();
}
