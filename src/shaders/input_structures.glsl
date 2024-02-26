layout (set = 0, binding = 0) uniform SceneData {
	mat4 view;
	mat4 proj;
	mat4 viewProj;
    vec4 cameraPos;
	vec4 ambientColor; // w - ambient intensity
	vec4 sunlightDirection;
	vec4 sunlightColor; // w - sun intensity
    vec4 cascadeFarPlaneZs;
    mat4 csmLightSpaceTMs[3]; // 3 = NUM_SHADOW_CASCADES
} sceneData;

layout (set = 0, binding = 1) uniform sampler2DArrayShadow csmShadowMap;

layout (set = 1, binding = 0) uniform MaterialData {
    vec4 baseColor;
    vec4 metallicRoughness;
} materialData;

layout (set = 1, binding = 1) uniform sampler2D diffuseTex;
