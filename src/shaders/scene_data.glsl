#include "light_data.glsl"

layout (set = 0, binding = 0) uniform SceneData {
	mat4 view;
	mat4 proj;
	mat4 viewProj;
    vec4 cameraPos;
	vec4 ambientColor; // w - ambient intensity
	vec4 sunlightDirection;
	vec4 sunlightColor; // w - sun intensity
    vec4 fogColorAndDensity; // w - for density
    vec4 cascadeFarPlaneZs;
    mat4 csmLightSpaceTMs[3]; // 3 = NUM_SHADOW_CASCADES
    int numLights;
} sceneData;

layout (set = 0, binding = 1) uniform sampler2DArrayShadow csmShadowMap;

layout (std140, set = 0, binding = 2) readonly buffer LightsDataBuffer {
    LightData lights[];
} lightsBuffer;

