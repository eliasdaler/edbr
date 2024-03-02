#pragma once

#include <deque>
#include <functional>

#include <vulkan/vulkan.h>

class DeletionQueue {
public:
    void pushFunction(std::function<void(VkDevice)>&& f);
    void flush(VkDevice device);

private:
    std::deque<std::function<void(VkDevice)>> deletors;
};
