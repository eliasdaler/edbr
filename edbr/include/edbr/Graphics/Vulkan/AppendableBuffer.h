#pragma once

#include <cassert>
#include <cstring> // memcpy
#include <span>

#include <vulkan/vulkan.h>

#include <edbr/Graphics/Vulkan/GPUBuffer.h>

template<typename T>
struct AppendableBuffer {
    void append(std::span<const T> elements)
    {
        assert(size + elements.size() <= capacity);
        auto arr = (T*)buffer.info.pMappedData;
        memcpy((void*)&arr[size], elements.data(), elements.size() * sizeof(T));
        size += elements.size();
    }

    void append(const T& element)
    {
        assert(size + 1 <= capacity);
        auto arr = (T*)buffer.info.pMappedData;
        memcpy((void*)&arr[size], &element, sizeof(T));
        ++size;
    }

    void clear() { size = 0; }

    VkBuffer getVkBuffer() const { return buffer.buffer; }

    GPUBuffer buffer;
    std::size_t capacity{};
    std::size_t size{0};
};
