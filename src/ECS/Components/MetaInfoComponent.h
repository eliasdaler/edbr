#pragma once

#include <string>

// This component is added automatically to each entity by EntityFactory
// and allows to add some metadata useful for debug tools
struct MetaInfoComponent {
    std::string sceneNodeName; // scene node name from gltf
    std::string prefabName;
};
