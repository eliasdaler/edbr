#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <edbr/ActionList/ActionListManager.h>
#include <edbr/Audio/IAudioManager.h>
#include <edbr/DevTools/ScreenshotTaker.h>
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
        glm::ivec2 windowSize{}; // default window size
        glm::ivec2 renderSize{}; // size of presented draw image

        std::string appName{"EDBR Application"};
        std::string windowTitle; // if not set, set to appName
        Version version{};

        std::string sourceSubDirName;
        // needed for finding dev files in source directory
        // they should be stored in ${EDBR_SOURCE_ROOT}/games/<sourceSubDirName>/dev/
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

    virtual void onWindowResize(){};

    const Version& getVersion() const { return params.version; }

    IAudioManager& getAudioManager();

    virtual ImageId getMainDrawImageId() const { return NULL_IMAGE_ID; }

protected:
    virtual void loadAppSettings(){};
    virtual void loadDevSettings(const std::filesystem::path& configPath);

    GfxDevice gfxDevice;

    SDL_Window* window{nullptr};

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

    InputManager inputManager;
    EventManager eventManager;
    ActionListManager actionListManager;
    TextManager textManager;

    ScreenshotTaker screenshotTaker;

private:
    void handleBaseDevInput();

    std::unique_ptr<IAudioManager> audioManager;
};
