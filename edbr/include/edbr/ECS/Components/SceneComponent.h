#pragma once

#include <string>

// This component is added automatically to each entity by EntityFactory
// and allows to add some metadata useful for debug tools
struct SceneComponent {
    std::string sceneName; // if set, prefab is loaded from that scene file
    std::string creationSceneName; // from which scene the entity was created
    std::string sceneNodeName; // scene node name from gltf
};