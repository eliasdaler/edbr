#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <entt/entity/registry.hpp>

#include <edbr/Application.h>

#include <edbr/Camera/CameraManager.h>
#include <edbr/ECS/EntityFactory.h>
#include <edbr/Graphics/Camera.h>
#include <edbr/Graphics/GameRenderer.h>
#include <edbr/Graphics/MaterialCache.h>
#include <edbr/Graphics/MeshCache.h>
#include <edbr/Graphics/Scene.h>
#include <edbr/Graphics/SkeletalAnimationCache.h>
#include <edbr/Graphics/SpriteRenderer.h>
#include <edbr/Save/SaveFileManager.h>
#include <edbr/SceneCache.h>

#include <edbr/DevTools/ActionListInspector.h>
#include <edbr/DevTools/EntityInfoDisplayer.h>
#include <edbr/DevTools/Im3dState.h>
#include <edbr/DevTools/ResourcesInspector.h>

#include "DevTools/CustomEntityTreeView.h"

#include "EntityCreator.h"
#include "GameUI.h"
#include "Level.h"

#include "AnimationSoundSystem.h"
#include "LevelScript.h"
#include "PhysicsSystem.h"

class ComponentFactory;
class FollowCameraController;

class MTPSaveFile;

class Game : public Application {
public:
    enum class GameState {
        Invalid,
        MainMenu,
        Playing,
        Cutscene,
        Paused,
    };

    enum class LevelTransitionType {
        Teleport, // no sounds played on transition
        EnterDoor, // door open/close sounds played on transition
    };

public:
    Game();

    void defineCLIArgs() override;
    void loadAppSettings() override;

    void customInit() override;
    void customUpdate(float dt) override;
    void customDraw() override;
    void customCleanup() override;

    void registerLevels();
    void initUI();
    void loadLevel(const std::filesystem::path& path);
    void changeLevel(
        const std::string& levelTag,
        const std::string& spawnName = {},
        LevelTransitionType ltt = LevelTransitionType::Teleport);
    void doLevelChange();
    ActionList enterLevel(LevelTransitionType ltt);
    ActionList exitLevel(LevelTransitionType ltt);

    void handleInput(float dt);
    void handlePlayerInput(float dt);
    void handlePlayerInteraction();

    entt::const_handle findDefaultCamera();
    void setCurrentCamera(entt::const_handle camera, float transitionTime = 0.f);
    void setCurrentCamera(const std::string& cameraTag, float transitionTime = 0.f);
    void setFollowCamera(float transitionTime = 0.f);
    FollowCameraController& getFollowCameraController();
    void cameraFollowEntity(entt::handle e, bool instantTeleport = true);

    void generateDrawList();

    MTPSaveFile& getSaveFile();
    void writeSaveFile();

    const std::string& getCurrentLevelName() const;
    const std::string& getLastSpawnName() const;

    const TextManager& getTextManager() const { return textManager; }
    entt::registry& getEntityRegistry() { return registry; }

    // high level functions for scripts
    void stopPlayerMovement();

    // dialogue
    [[nodiscard]] ActionList say(const LocalizedStringTag& text, entt::handle speaker = {});
    [[nodiscard]] ActionList say(const dialogue::TextToken& textToken, entt::handle speaker = {});
    [[nodiscard]] ActionList say(
        std::span<const dialogue::TextToken> textTokens,
        entt::handle speaker = {});

    [[nodiscard]] ActionList saveGameSequence();
    void playSound(const std::string& soundName);

    // Do task with some delay. The taskName should be unique for each task
    void doWithDelay(const std::string& taskName, float delay, std::function<void()> task);

public:
    // game states
    void enterMainMenu();
    void startNewGame();
    void startLoadedGame();
    void exitGame();
    void startGameplay();
    void enterPauseMenu();
    void exitPauseMenu();
    void exitToTitle();

    const std::size_t NumSaveFiles = 3;
    bool hasAnySaveFile() const;

private:
    void initEntityFactory();
    void registerComponents(ComponentFactory& componentFactory);
    void registerComponentDisplayers();

    template<typename T>
    void registerLevelScript(std::string name)
    {
        auto [it, inserted] = levelScripts.emplace(std::move(name), std::make_unique<T>(*this));
        assert(inserted);
    }
    LevelScript& getLevelScript();

    void handleSaveFiles();

    glm::ivec2 getGameScreenSize() const;
    glm::vec2 getMouseGameScreenCoord() const;
    void updateGameLogic(float dt);

    void entityPostInit(entt::handle e);
    void initEntityAnimation(entt::handle e);

    void destroyNonPersistentEntities();
    void destroyEntity(entt::handle e);

    void onCollisionStarted(const CharacterCollisionStartedEvent& event);
    void onCollisionEnded(const CharacterCollisionEndedEvent& event);

    void devToolsHandleInput(float dt);
    void devToolsNewFrame(float dt);
    void devToolsUpdate(float dt);
    void devToolsDrawUI(SpriteRenderer& spriteRenderer);

private:
    MeshCache meshCache;
    MaterialCache materialCache;
    GameRenderer renderer;
    SpriteRenderer spriteRenderer;

    GameState gameState{GameState::Invalid};
    // position and size of where the game is rendered in the window
    glm::ivec2 gameWindowPos;
    glm::ivec2 gameWindowSize;

    SaveFileManager saveFileManager;

    SceneCache sceneCache;
    SkeletalAnimationCache animationCache;

    entt::registry registry;
    EntityFactory entityFactory;
    EntityCreator entityCreator;

    std::unique_ptr<PhysicsSystem> physicsSystem;
    AnimationSoundSystem animationSoundSystem;

    std::filesystem::path skyboxDir;

    Level level;
    std::unordered_map<std::string, std::unique_ptr<LevelScript>> levelScripts;
    std::string newLevelToLoad; // if set, will load this new level during the end of update
    std::string newLevelSpawnName;
    std::string lastSpawnName;
    std::string levelTransitionListName{"level_transition"};
    LevelTransitionType newLevelTransitionType{LevelTransitionType::Teleport};

    entt::handle interactEntity;

    CameraManager cameraManager;
    Camera camera;
    std::string previousCameraControllerTag;
    Transform previousCameraTransform;
    std::string freeCameraControllerTag{"free"};
    std::string followCameraControllerTag{"follow"};
    float cameraNear{1.f};
    float cameraFar{200.f};
    float cameraFovX{glm::radians(45.f)};

    bool playerInputEnabled{true};

    GameUI ui;

    // DEV
    bool orbitCameraAroundSelectedEntity{false};
    bool freeCameraMode{false};
    std::string devLevelToLoad;
    std::string devStartSpawnPoint;

    CustomEntityTreeView entityTreeView;
    EntityInfoDisplayer entityInfoDisplayer;
    ResourcesInspector resourcesInspector;
    ActionListInspector actionListInspector;

    // only display update FPS every 1 seconds, otherwise it's too noisy
    float displayedFPS{0.f};
    float displayFPSDelay{1.f};

    bool gameDrawnInWindow{false};
    const char* gameWindowLabel{"Game##window"};
    bool drawEntityTags{false};
    bool drawEntityHeading{false};
    bool drawImGui{true};
    bool drawGizmos{false};

    bool devPaused{false};
    bool devResumeForOneFrame{false};

    Im3dState im3d;
};
