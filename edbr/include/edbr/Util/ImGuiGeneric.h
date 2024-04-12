#pragma once

#include <glm/glm.hpp>

#include <edbr/Math/Rect.h>

namespace ImGui
{
bool Input(const char* label, int* obj);
bool Input(const char* label, float* obj);

bool Drag(const char* label, int* obj);
bool Drag(const char* label, float* obj);

template<typename T, glm::length_t N>
bool Input(const char* label, glm::vec<N, T, glm::packed_highp>* v);

template<typename T>
bool Input(const char* label, math::Rect<T>* obj);

template<typename T, glm::length_t N>
bool Input(const char* label, T* arr);

template<typename T, glm::length_t N>
bool Drag(
    const char* label,
    glm::vec<N, T, glm::packed_highp>* v,
    float speed = 1.f,
    const glm::vec<N, T, glm::packed_highp>* min = nullptr,
    const glm::vec<N, T, glm::packed_highp>* max = nullptr);

template<typename T>
bool Drag(const char* label, math::Rect<T>* obj);

template<typename T, glm::length_t N>
bool Drag(
    const char* label,
    T* arr,
    float speed = 1.f,
    const T* min = nullptr,
    const T* max = nullptr);
}

#include "ImGuiGeneric.inl"
