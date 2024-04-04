#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/EntityFactory.h>
#include <edbr/Graphics/SkeletalAnimationCache.h>
#include <edbr/SceneCache.h>

#include "Components.h"
#include "EntityCreator.h"
#include "GameSceneLoader.h"

namespace
{
class SceneInitTest : public ::testing::Test {
protected:
    SceneInitTest() : sceneCache(animationCache) {}

    void registerEmptyPrefab(const std::string& prefabName)
    {
        nlohmann::json data{};
        JsonFile file(std::move(data));
        entityFactory.addPrefabFile(prefabName, file);
    }

    void registerCatPrefab()
    {
        nlohmann::json data{};
        data["meta"] = {
            {"scene", "cato.gltf"},
        };
        JsonFile file(std::move(data));
        entityFactory.addPrefabFile("cat", file);
    }

    void SetUp() override
    {
        registerCatPrefab();
        registerEmptyPrefab("funny_frog");
        registerEmptyPrefab("static_geometry");
        registerEmptyPrefab("point");
        registerEmptyPrefab("light");
        registerEmptyPrefab("camera");

        entityFactory.setCreateDefaultEntityFunc([](entt::registry& registry) {
            auto e = registry.create();
            registry.emplace<TransformComponent>(e);
            registry.emplace<HierarchyComponent>(e);
            return entt::handle(registry, e);
        });
        entityFactory.addMappedPrefabName("frog", "funny_frog");
        entityFactory.addMappedPrefabName("froggy", "frog"); // multiple levels

        auto& componentFactory = entityFactory.getComponentFactory();
        componentFactory.registerComponentLoader(
            "meta", [](entt::handle e, MetaInfoComponent& mic, const JsonDataLoader& loader) {
                loader.getIfExists("scene", mic.sceneName);
                loader.getIfExists("node", mic.sceneNodeName);
            });

        { // cato scene
            Scene scene;
            scene.meshes.push_back(SceneMesh{.primitives = {0, 1, 2}});
            {
                auto catNode = std::make_unique<SceneNode>(SceneNode{
                    .name = "Cat",
                    .meshIndex = 0,
                });

                auto pointNode = std::make_unique<SceneNode>(SceneNode{
                    .name = "Point.Mouth",
                });
                pointNode->transform.setPosition(glm::vec3{0.f, 0.5f, 0.f});
                catNode->children.push_back(std::move(pointNode));

                scene.nodes.push_back(std::move(catNode));
            }
            sceneCache.addScene("cato.gltf", std::move(scene));
        }

        { // scene.gltf
            Scene scene;
            scene.meshes.push_back(SceneMesh{.primitives = {2, 3, 4}});
            scene.meshes.push_back(SceneMesh{.primitives = {5, 6}});
            scene.meshes.push_back(SceneMesh{.primitives = {7, 8}});

            Light light{
                .type = LightType::Directional,
                .intensity = 0.5f,
            };
            scene.lights.push_back(light);

            {
                auto catNode = std::make_unique<SceneNode>(SceneNode{
                    .name = "Cat.Epic.42",
                    .meshIndex = 0,
                });
                catNode->transform.setPosition(glm::vec3{0.f, 0.f, 1.f});
                scene.nodes.push_back(std::move(catNode));
            }

            { // second cat with child point
                auto catNode = std::make_unique<SceneNode>(SceneNode{
                    .name = "Cat.With.Child.Point",
                    .meshIndex = 0,
                });
                catNode->transform.setPosition(glm::vec3{0.f, 0.f, 2.f});

                auto pointNode = std::make_unique<SceneNode>(SceneNode{
                    .name = "Point.Test",
                });
                pointNode->transform.setPosition(glm::vec3{0.f, 1.f, 0.f});
                catNode->children.push_back(std::move(pointNode));

                scene.nodes.push_back(std::move(catNode));
            }

            { // mesh node with a child which will be merged into parent
                auto meshNode = std::make_unique<SceneNode>(SceneNode{
                    .name = "Mesh.Test",
                    .meshIndex = 1,
                });
                auto meshChildNode = std::make_unique<SceneNode>(SceneNode{
                    .name = "Mesh.Test.Child",
                    .meshIndex = 2,
                });
                meshNode->children.push_back(std::move(meshChildNode));
                scene.nodes.push_back(std::move(meshNode));
            }

            { // light node
                auto lightNode = std::make_unique<SceneNode>(SceneNode{
                    .name = "Sun",
                    .lightId = 0,
                });
                lightNode->transform.setPosition(glm::vec3{0.f, 10.f, 10.f});
                scene.nodes.push_back(std::move(lightNode));
            }

            sceneCache.addScene("scene.gltf", std::move(scene));
        }
    }

    void TearDown() override {}

    SkeletalAnimationCache animationCache;
    SceneCache sceneCache;
    EntityFactory entityFactory;
};

}

TEST_F(SceneInitTest, TestGetPrefabNameFromSceneNode)
{
    auto& scene = sceneCache.getScene("scene.gltf");
    struct testData {
        const char* nodeName;
        const char* expectedPrefabName;
    };
    auto tests = std::vector<testData>{
        // simple cases
        {"Cat", "cat"},
        {"Cat.2", "cat"}, // remove everything after first dot
        {"Cat.epic.2", "cat"}, // remove everything after first dot
        {"cat.2", "cat"}, // in lowercase already
        // PascalCase is converted to snake_case on lookup
        {"FunnyFrog", "funny_frog"},
        {"FunnyFrog.2", "funny_frog"},
        {"funny_frog.Epic.2", "funny_frog"}, // already in snake_case
        // "frog" is mapped to "funny_frog"
        {"Frog.Epic.2", "funny_frog"},
        {"frog.Epic.2", "funny_frog"},
        // "froggy" -> "frog" -> "funny_frog"
        {"Froggy.Epic.2", "funny_frog"},
        // name not found
        {"Unknown", ""},
        {"Unknown.2", ""},
    };
    for (const auto& t : tests) {
        EXPECT_EQ(
            util::getPrefabNameFromSceneNode(entityFactory, t.nodeName), t.expectedPrefabName);
    }
}

TEST_F(SceneInitTest, TestCreateEntitiesBasic)
{
    using namespace testing;

    entt::registry registry;
    EntityCreator loader{
        .registry = registry,
        .defaultPrefabName = "static_geometry",
        .entityFactory = entityFactory,
        .sceneCache = sceneCache,
    };

    auto& scene = sceneCache.getScene("scene.gltf");
    auto entities = loader.createEntitiesFromScene(scene);
    EXPECT_EQ(entities.size(), 4);

    { // first cat, just regular prefab
        auto& cat = entities[0];
        auto& mic = cat.get<MetaInfoComponent>();
        EXPECT_EQ(mic.sceneName, "cato.gltf");
        EXPECT_EQ(mic.sceneNodeName, "Cat.Epic.42");
        EXPECT_EQ(mic.prefabName, "cat");

        auto& tc = cat.get<TransformComponent>();
        glm::vec3 pos{0.f, 0.f, 1.f};
        EXPECT_EQ(tc.transform.getPosition(), pos);

        auto& mc = cat.get<MeshComponent>();
        ASSERT_THAT(mc.meshes, ElementsAre(0, 1, 2));
    }

    { // cat with child point
        const auto& cat = entities[1];
        const auto& mic = cat.get<MetaInfoComponent>();
        EXPECT_EQ(mic.sceneName, "cato.gltf");
        EXPECT_EQ(mic.creationSceneName, "scene.gltf");
        EXPECT_EQ(mic.sceneNodeName, "Cat.With.Child.Point");
        EXPECT_EQ(mic.prefabName, "cat");

        const auto& tc = cat.get<TransformComponent>();
        glm::vec3 pos{0.f, 0.f, 2.f};
        EXPECT_EQ(tc.transform.getPosition(), pos);

        const auto& mc = cat.get<MeshComponent>();
        ASSERT_THAT(mc.meshes, ElementsAre(0, 1, 2));

        // check that child was added
        const auto& hc = cat.get<HierarchyComponent>();
        EXPECT_EQ(hc.children.size(), 2);

        { // child from prefab scene
            const auto& point = hc.children[0];
            const auto& mic = point.get<MetaInfoComponent>();
            EXPECT_EQ(mic.sceneName, "");
            EXPECT_EQ(mic.creationSceneName, "cato.gltf");
            EXPECT_EQ(mic.sceneNodeName, "Point.Mouth");
            EXPECT_EQ(mic.prefabName, "point");

            const auto& tc = point.get<TransformComponent>();
            glm::vec3 pos{0.f, 0.5f, 0.f};
            EXPECT_EQ(tc.transform.getPosition(), pos);
        }

        { // child from scene.gltf
            const auto& point = hc.children[1];
            const auto& mic = point.get<MetaInfoComponent>();
            EXPECT_EQ(mic.sceneName, "");
            EXPECT_EQ(mic.creationSceneName, "scene.gltf");
            EXPECT_EQ(mic.sceneNodeName, "Point.Test");
            EXPECT_EQ(mic.prefabName, "point");

            const auto& tc = point.get<TransformComponent>();
            glm::vec3 pos{0.f, 1.f, 0.f};
            EXPECT_EQ(tc.transform.getPosition(), pos);
        }
    }

    { // mesh has a child merged into it
        const auto& mesh = entities[2];
        const auto& mic = mesh.get<MetaInfoComponent>();
        EXPECT_EQ(mic.sceneName, "");
        EXPECT_EQ(mic.creationSceneName, "scene.gltf");
        EXPECT_EQ(mic.sceneNodeName, "Mesh.Test");
        EXPECT_EQ(mic.prefabName, "static_geometry");

        const auto& mc = mesh.get<MeshComponent>();
        // Merged meshes {7, 8} FROM Mesh.Test.Child
        ASSERT_THAT(mc.meshes, ElementsAre(5, 6, 7, 8));
    }

    { // light
        const auto& lightEntity = entities[3];
        const auto& mic = lightEntity.get<MetaInfoComponent>();
        EXPECT_EQ(mic.sceneName, "");
        EXPECT_EQ(mic.creationSceneName, "scene.gltf");
        EXPECT_EQ(mic.sceneNodeName, "Sun");
        EXPECT_EQ(mic.prefabName, "light");

        const auto& tc = lightEntity.get<TransformComponent>();
        glm::vec3 pos{0.f, 10.f, 10.f};
        EXPECT_EQ(tc.transform.getPosition(), pos);

        const auto& light = lightEntity.get<LightComponent>().light;
        Light expectedLight{
            .type = LightType::Directional,
            .intensity = 0.5f,
        };
        // TODO: operator== for light
        EXPECT_EQ(light.type, expectedLight.type);
        EXPECT_EQ(light.intensity, expectedLight.intensity);
    }
}
