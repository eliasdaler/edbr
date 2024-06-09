#include <edbr/Application.h>

#include <edbr/Audio/AudioManager.h>
#include <edbr/Audio/NullAudioManager.h>
#include <edbr/Core/JsonFile.h>

#include <chrono>

#include <SDL2/SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include <tracy/Tracy.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

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

void Application::defineCLIArgs()
{
    cliApp.add_flag("-p,--prod", prodMode, "Run in prod mode (even when dev path is set)");
}
void Application::parseCLIArgs(int argc, char** argv)
{
    try {
        cliApp.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        std::exit(cliApp.exit(e));
    }
}

void Application::init(const Params& ps)
{
    params = ps;

#ifdef _WIN32
    // This won't be needed in SDL 3.0.
    // But without this, the application is scaled to whatever the scale
    // the user has set - but it shouldn't be scaled that way.
    BOOL dpi_result = SetProcessDPIAware();
    assert(dpi_result == TRUE);
#endif

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("SDL could not initialize. SDL Error: %s\n", SDL_GetError());
        std::exit(1);
    }

    if (!prodMode) {
        // Set EDBR_DEV_PATH to a path containing dev_settings.json to enable dev mode
        if (auto* devSettingsPath = std::getenv("EDBR_DEV_PATH"); devSettingsPath) {
            isDevEnvironment = true;
            const auto configPath = std::filesystem::path{devSettingsPath};
            devDirPath = configPath.parent_path();
            loadDevSettings(std::filesystem::path{devSettingsPath});
        }
    }

    loadAppSettings();

    if (params.windowSize == glm::ivec2{}) {
        assert(params.renderSize != glm::ivec2{});
        params.windowSize = params.renderSize;
    }

    if (params.renderSize == glm::ivec2{}) {
        assert(params.windowSize != glm::ivec2{});
        params.renderSize = params.windowSize;
    }

    if (params.windowTitle.empty()) {
        params.windowTitle = params.appName;
    }

    window = SDL_CreateWindow(
        params.windowTitle.c_str(),
        // pos
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        // size
        params.windowSize.x,
        params.windowSize.y,
        SDL_WINDOW_VULKAN);

    SDL_SetWindowResizable(window, SDL_TRUE);

    if (!window) {
        printf("Failed to create window. SDL Error: %s\n", SDL_GetError());
        std::exit(1);
    }

    gfxDevice.init(window, params.appName.c_str(), params.version, vSync);

    if (!devDirPath.empty()) {
        imguiIniPath = (devDirPath / "imgui.ini").string();
        ImGui::GetIO().IniFilename = imguiIniPath.c_str();
        fmt::println("ImGui ini path: {}, {}", devDirPath.string(), ImGui::GetIO().IniFilename);
    } else {
        // don't write imgui.ini
        ImGui::GetIO().IniFilename = nullptr;
    }

    audioManager = std::make_unique<AudioManager>();
    if (!audioManager->init()) {
        fmt::println("[error] failed to init audio: using null audio manager instead");
        audioManager = std::make_unique<NullAudioManager>();
        assert(audioManager->init());
    }

    customInit();
}

void Application::loadDevSettings(const std::filesystem::path& configPath)
{
    JsonFile file(configPath);
    if (!file.isGood()) {
        fmt::println("failed to load dev settings from {}", configPath.string());
        return;
    }

    const auto loader = file.getLoader();
    loader.getIfExists("window", params.windowSize);
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

        if (frameTime > 0.07f && frameTime < 5.f) { // if >=5.f - debugging?
            printf("frame drop, time: %.4f\n", frameTime);
        }

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
                    if (event.type == SDL_WINDOWEVENT) {
                        switch (event.window.event) {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            /* fallthrough */
                        case SDL_WINDOWEVENT_RESIZED:
                            fmt::println(
                                "window resized: {}x{}", event.window.data1, event.window.data2);
                            params.windowSize = {event.window.data1, event.window.data2};
                            break;
                        }
                    }
                    inputManager.handleEvent(event);
                    ImGui_ImplSDL2_ProcessEvent(&event);
                }
            }

            if (gfxDevice.needsSwapchainRecreate()) {
                gfxDevice.recreateSwapchain(params.windowSize.x, params.windowSize.y);
            }

            // ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            // update
            inputManager.update(dt);
            customUpdate(dt);

            actionListManager.update(dt, gamePaused);
            eventManager.update();
            audioManager->update();

            accumulator -= dt;

            ImGui::Render();
        }

        if (!gfxDevice.needsSwapchainRecreate()) {
            customDraw();
        }
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

    audioManager->cleanup();

    gfxDevice.cleanup();
    inputManager.cleanup();

    SDL_DestroyWindow(window);
    SDL_Quit();
}

IAudioManager& Application::getAudioManager()
{
    assert(audioManager && "audio wasn't initialized");
    return *audioManager;
}
