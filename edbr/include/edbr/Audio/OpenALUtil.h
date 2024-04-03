#pragma once

#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <AL/al.h>
#include <AL/alc.h>
#ifndef __EMSCRIPTEN__
#include <AL/efx-presets.h>
#endif

#define alCall(function, ...) alCallImpl(__FILE__, __LINE__, function, __VA_ARGS__)
#define alcCall(function, device, ...) \
    alcCallImpl(__FILE__, __LINE__, function, device, __VA_ARGS__)

bool check_al_errors(const char* filename, const std::uint32_t line);
bool check_alc_errors(const char* filename, const std::uint_fast32_t line, ALCdevice* device);

template<typename alFunction, typename... Params>
auto alCallImpl(
    const char* filename,
    const std::uint32_t line,
    alFunction function,
    Params&&... params) -> typename std::
    enable_if_t<!std::is_same_v<void, decltype(function(params...))>, decltype(function(params...))>
{
    auto ret = function(std::forward<Params>(params)...);
    check_al_errors(filename, line);
    return ret;
}

template<typename alFunction, typename... Params>
auto alCallImpl(
    const char* filename,
    const std::uint32_t line,
    alFunction function,
    Params&&... params) ->
    typename std::enable_if_t<std::is_same_v<void, decltype(function(params...))>, bool>
{
    function(std::forward<Params>(params)...);
    return check_al_errors(filename, line);
}

template<typename alcFunction, typename... Params>
auto alcCallImpl(
    const char* filename,
    const std::uint32_t line,
    alcFunction function,
    ALCdevice* device,
    Params&&... params) ->
    typename std::enable_if_t<std::is_same_v<void, decltype(function(params...))>, bool>
{
    function(std::forward<Params>(params)...);
    return check_alc_errors(filename, line, device);
}

template<typename alcFunction, typename ReturnType, typename... Params>
auto alcCallImpl(
    const char* filename,
    const std::uint32_t line,
    alcFunction function,
    ReturnType& returnValue,
    ALCdevice* device,
    Params&&... params) ->
    typename std::enable_if_t<!std::is_same_v<void, decltype(function(params...))>, bool>
{
    returnValue = function(std::forward<Params>(params)...);
    return check_alc_errors(filename, line, device);
}

namespace util
{
void setOpenALListenerPosition(float x, float y, float z);
// orientation = {f_x, f_y, f_z, u_x, u_y, u_z}, where f - forward vector, u - up vector
void setOpenALListenerOrientation(const std::array<float, 6>& orientation);

#ifndef __EMSCRIPTEN__
ALuint LoadEffect(const EFXEAXREVERBPROPERTIES* reverb);
#endif
}
