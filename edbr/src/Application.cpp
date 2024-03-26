#include <edbr/Application.h>

#include <chrono>

#include <SDL2/SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include <tracy/Tracy.hpp>

#ifdef TRACY_ENABLE
void* operator new(std ::size_t count)
{
    auto ptr = malloc(count);
    TracyAlloc(ptr, count);
    return ptr;
}
void operator delete(void* ptr) noexcept
{
    TracyFree(ptr);
    free(ptr);
}
void operator delete(void* ptr, std::size_t) noexcept
{
    TracyFree(ptr);
    free(ptr);
}
#endif

void Application::init(const Params& ps)
{
    params = ps;
    if (params.renderWidth == 0 && params.renderHeight == 0) {
        params.renderWidth = params.windowWidth;
        params.renderHeight = params.windowHeight;
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("SDL could not initialize. SDL Error: %s\n", SDL_GetError());
        std::exit(1);
    }

    window = SDL_CreateWindow(
        ps.title,
        // pos
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        // size
        ps.windowWidth,
        ps.windowHeight,
        SDL_WINDOW_VULKAN);

    if (!window) {
        printf("Failed to create window. SDL Error: %s\n", SDL_GetError());
        std::exit(1);
    }

    gfxDevice.init(window, ps.title, vSync);

    customInit();
}

void Application::run()
{
    // Fix your timestep! game loop
    const float FPS = 60.f;
    const float dt = 1.f / FPS;

    auto prevTime = std::chrono::high_resolution_clock::now();
    float accumulator = dt; // so that we get at least 1 update before render

    isRunning = true;
    while (isRunning) {
        const auto newTime = std::chrono::high_resolution_clock::now();
        frameTime = std::chrono::duration<float>(newTime - prevTime).count();

        accumulator += frameTime;
        prevTime = newTime;

        // moving average
        float newFPS = 1.f / frameTime;
        if (newFPS == std::numeric_limits<float>::infinity()) {
            // can happen when frameTime == 0
            newFPS = 0;
        }
        avgFPS = std::lerp(avgFPS, newFPS, 0.1f);

        if (accumulator > 10 * dt) { // game stopped for debug
            accumulator = dt;
        }

        while (accumulator >= dt) {
            ZoneScopedN("Tick");
            inputManager.onNewFrame();
            { // event processing
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        isRunning = false;
                        return;
                    }
                    inputManager.handleEvent(event);
                    ImGui_ImplSDL2_ProcessEvent(&event);
                }
            }

            // ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            // update
            inputManager.update(dt);
            customUpdate(dt);

            accumulator -= dt;

            ImGui::Render();
        }

        customDraw();
        FrameMark;

        if (frameLimit) {
            // Delay to not overload the CPU
            const auto now = std::chrono::high_resolution_clock::now();
            const auto frameTime = std::chrono::duration<float>(now - prevTime).count();
            if (dt > frameTime) {
                SDL_Delay(static_cast<std::uint32_t>(dt - frameTime));
            }
        }
    }
}

void Application::cleanup()
{
    customCleanup();

    gfxDevice.cleanup();
    inputManager.cleanup();

    SDL_DestroyWindow(window);
    SDL_Quit();
}
