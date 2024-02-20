#version 460

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform SceneData{
	mat4 view;
	mat4 proj;
	mat4 viewProj;
    vec4 cameraPos;
	vec4 ambientColor; // w - ambient intensity
	vec4 sunlightDirection;
	vec4 sunlightColor; // w - sun intensity
} sceneData;

layout (set = 1, binding = 0) uniform sampler2D diffuseTex;

float calculateSpecularBP(float NoH) {
    float shininess = 32.0 * 4.0;
    return pow(NoH, shininess);
}

vec3 blinnPhongBRDF(vec3 diffuse, vec3 n, vec3 v, vec3 l, vec3 h) {
    vec3 Fd = diffuse;

    // specular
    // TODO: read from spec texture / pass spec param
    vec3 specularColor = diffuse * 0.5;
    float NoH = clamp(dot(n, h), 0, 1);
    vec3 Fr = specularColor * calculateSpecularBP(NoH);

    return Fd + Fr;
}


void main()
{
    vec3 diffuse = texture(diffuseTex, inUV).rgb;

    vec3 cameraPos = sceneData.cameraPos.xyz;

    vec3 n = normalize(inNormal);
    vec3 l = sceneData.sunlightDirection.xyz;
    vec3 v = normalize(cameraPos - inPos);
    vec3 h = normalize(v + l);
    float NoL = clamp(dot(n, l), 0, 1);

    vec3 fr = blinnPhongBRDF(diffuse, n, v, l, h);

    vec3 fragColor = (fr * sceneData.sunlightColor.xyz) * (sceneData.sunlightColor.w * NoL);

    // ambient
    fragColor += diffuse * sceneData.ambientColor.xyz * sceneData.ambientColor.w;

    // fake gamma correction
    fragColor = pow(fragColor, vec3(1.f/2.2f));

	outFragColor = vec4(fragColor, 1.0f);
}
