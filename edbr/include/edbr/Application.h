#pragma once

#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Input/InputManager.h>

struct SDL_Window;

class Application {
public:
    struct Params {
        int windowWidth{640};
        int windowHeight{480};
        int renderWidth{};
        int renderHeight{};
        const char* title{"EDBR Application"};
    };

public:
    void init(const Params& ps);
    void run();
    void cleanup();

    virtual void customInit() = 0;
    virtual void customUpdate(float dt) = 0;
    virtual void customDraw() = 0;
    virtual void customCleanup() = 0;

protected:
    GfxDevice gfxDevice;

    SDL_Window* window;

    Params params;
    bool vSync{true};

    bool isRunning{false};
    bool frameLimit{true};
    float frameTime{0.f};
    float avgFPS{0.f};

    InputManager inputManager;
};
