#pragma once

#include <SDL2/SDL.h>
#include <webgpu/webgpu_cpp.h>

namespace util
{
wgpu::Surface CreateSurfaceForSDLWindow(const wgpu::Instance& instance, SDL_Window* window);
}