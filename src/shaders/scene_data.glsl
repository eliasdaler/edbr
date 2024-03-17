#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_buffer_reference : require

#include "light.glsl"

layout (buffer_reference, scalar) readonly buffer LightsDataBuffer {
    Light data[];
};

struct MaterialData {
    vec4 baseColor;
    vec4 metallicRoughnessEmissive;
    uint diffuseTex;
    uint normalTex;
    uint metallicRoughnessTex;
    uint emissiveTex;
};

layout (buffer_reference, std430) readonly buffer MaterialsBuffer {
    MaterialData data[];
} materialsBuffer;

layout (set = 1, binding = 0, scalar) uniform SceneDataBlock {
    // camera
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 cameraPos;

    // ambient
    vec3 ambientColor;
    float ambientIntensity;

    // fog
    vec3 fogColor;
    float fogDensity;

    // CSM data
    vec4 cascadeFarPlaneZs;
    mat4 csmLightSpaceTMs[3]; // 3 = NUM_SHADOW_CASCADES
    uint csmShadowMapId;

    LightsDataBuffer lights;
    int numLights;
    int sunlightIndex; // if -1, there's no sun

    MaterialsBuffer materials;
} sceneData;
