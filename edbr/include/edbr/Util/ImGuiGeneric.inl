#include <imgui.h>

#include <type_traits>

#include <glm/gtc/type_ptr.hpp>

#include <edbr/Math/Rect.h>

namespace ImGui
{
template<typename T, glm::length_t N>
bool Input(const char* label, glm::vec<N, T, glm::packed_highp>* v)
{
    return ImGui::Input<T, N>(label, glm::value_ptr(*v));
}

template<typename T, glm::length_t N>
bool Drag(
    const char* label,
    glm::vec<N, T, glm::packed_highp>* v,
    float speed,
    const glm::vec<N, T, glm::packed_highp>* min,
    const glm::vec<N, T, glm::packed_highp>* max)
{
    return ImGui::Drag<T, N>(
        label,
        glm::value_ptr(*v),
        speed,
        min ? glm::value_ptr(*min) : nullptr,
        max ? glm::value_ptr(*max) : nullptr);
}

template<typename T>
bool Input(const char* label, math::Rect<T>* obj)
{
    T arr[] = {obj->left, obj->top, obj->width, obj->height};
    if (Input<T, 4>(label, arr)) {
        obj->left = arr[0];
        obj->top = arr[1];
        obj->width = arr[2];
        obj->height = arr[3];
        return true;
    }
    return false;
}

template<typename T>
bool Drag(const char* label, math::Rect<T>* obj)
{
    T arr[] = {obj->left, obj->top, obj->width, obj->height};
    if (Drag<T, 4>(label, arr)) {
        obj->left = arr[0];
        obj->top = arr[1];
        obj->width = arr[2];
        obj->height = arr[3];
        return true;
    }
    return false;
}

template<class T>
struct dependent_false : std::false_type {};

template<typename T>
constexpr ImGuiDataType getDataType()
{
    if constexpr (std::is_same_v<T, int>) {
        return ImGuiDataType_S32;
    } else if constexpr (std::is_same_v<T, float>) {
        return ImGuiDataType_Float;
    } else {
        static_assert(dependent_false<T>::value, "not implemented for other types");
    }
    return ImGuiDataType_COUNT;
}

template<typename T>
const char* getFormatString()
{
    if constexpr (std::is_same_v<T, int>) {
        return "%d";
    } else if constexpr (std::is_same_v<T, float>) {
        return "%.3f";
    } else {
        static_assert(dependent_false<T>::value, "not implemented for other types");
    }
    return nullptr;
}

template<typename T, glm::length_t N>
bool Input(const char* label, T* arr)
{
    return ImGui::InputScalarN(label, getDataType<T>(), arr, N);
}

template<typename T, glm::length_t N>
bool Drag(const char* label, T* arr, float speed, const T* min, const T* max)
{
    return ImGui::DragScalarN(
        label, getDataType<T>(), arr, N, speed, (void*)min, (void*)max, getFormatString<T>());
}

}
