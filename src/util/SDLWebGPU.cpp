#include "util/SDLWebGPU.h"

#include <webgpu/webgpu_cpp.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

namespace util
{
wgpu::Surface CreateSurfaceForSDLWindow(const wgpu::Instance& instance, SDL_Window* window)
{
    SDL_SysWMinfo windowWMInfo;
    SDL_VERSION(&windowWMInfo.version);
    SDL_GetWindowWMInfo(window, &windowWMInfo);

#if defined(SDL_VIDEO_DRIVER_X11)
    {
        Display* x11_display = windowWMInfo.info.x11.display;
        Window x11_window = windowWMInfo.info.x11.window;

        wgpu::SurfaceDescriptorFromXlibWindow x11SurfDesc;
        x11SurfDesc.display = x11_display;
        x11SurfDesc.window = x11_window;

        wgpu::SurfaceDescriptor surfaceDesc;
        surfaceDesc.nextInChain = &x11SurfDesc;
        return instance.CreateSurface(&surfaceDesc);
    }
#elif defined(SDL_VIDEO_DRIVER_WINDOWS)
    {
        HWND hwnd = windowWMInfo.info.win.window;
        HINSTANCE hinstance = GetModuleHandle(NULL);

        wgpu::SurfaceDescriptorFromWindowsHWND windowsSurfDesc;
        windowsSurfDesc.hwnd = hwnd;
        windowsSurfDesc.hinstance = hinstance;

        wgpu::SurfaceDescriptor surfaceDesc;
        surfaceDesc.nextInChain = &windowsSurfDesc;
        return instance.CreateSurface(&surfaceDesc);
    }
#else
#error "Unsupported WGPU_TARGET"
#endif
}
} // end of namespace util