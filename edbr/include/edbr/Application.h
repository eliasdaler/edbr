#pragma once

#include <edbr/ActionList/ActionListManager.h>
#include <edbr/Audio/AudioManager.h>
#include <edbr/Event/EventManager.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Input/InputManager.h>
#include <edbr/Text/TextManager.h>
#include <edbr/Version.h>

#include <glm/vec2.hpp>

#include <CLI/CLI.hpp>

struct SDL_Window;

class Application {
public:
    struct Params {
        // Either windowSize or renderSize should be set
        // You can also load them in loadAppSettings before they're used
        glm::ivec2 windowSize{};
        glm::ivec2 renderSize{};

        std::string appName{"EDBR Application"};
        std::string windowTitle; // if not set, set to appName
        Version version{};
    };

public:
    virtual void defineCLIArgs();
    void parseCLIArgs(int argc, char** argv);

    void init(const Params& ps);
    void run();
    void cleanup();

    virtual void customInit() = 0;
    virtual void customUpdate(float dt) = 0;
    virtual void customDraw() = 0;
    virtual void customCleanup() = 0;

    const Version& getVersion() const { return params.version; }

protected:
    virtual void loadAppSettings(){};
    virtual void loadDevSettings(const std::filesystem::path& configPath);

    GfxDevice gfxDevice;

    SDL_Window* window;

    Params params;
    bool vSync{true};

    bool isRunning{false};
    bool gamePaused{false};

    bool prodMode{false}; // if true - ignores dev env vars
    bool isDevEnvironment{false};
    std::string imguiIniPath;
    std::filesystem::path devDirPath;

    bool frameLimit{true};
    float frameTime{0.f};
    float avgFPS{0.f};

    CLI::App cliApp{};

    AudioManager audioManager;
    InputManager inputManager;
    EventManager eventManager;
    ActionListManager actionListManager;
    TextManager textManager;
};
