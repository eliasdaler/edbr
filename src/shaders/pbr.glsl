#define PI 3.1415926535897932384626433832795

// Trowbridge-Reitz GGX
float D_GGX(float NoH, float a) {
    float a2 = a * a;
    float f = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / (PI * f * f);
}

// Fresnel-Schlick approximation
vec3 F_Schlick(float HoV, vec3 f0) {
    return f0 + (vec3(1.0) - f0) * pow(1.0 - HoV, 5.0);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float a) {
    float a2 = a * a;
    float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return 0.5 / (GGXV + GGXL);
}

float Fd_Lambert() {
    return 1.0 / PI;
}

vec3 pbrBRDF(
        vec3 diffuse, float roughness, float metallic, vec3 f0,
        vec3 n, vec3 v, vec3 l, vec3 h)
{
    float NoV = abs(dot(n, v));
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float HoV = clamp(dot(h, v), 0.0, 1.0);

    float D = D_GGX(NoH, roughness);
    vec3  F = F_Schlick(HoV, f0);
    float V = V_SmithGGXCorrelated(NoV, NoL, roughness);

    // specular BRDF
    vec3 Fr = (D * V) * F;

    // diffuse BRDF
    vec3 Fd = diffuse * Fd_Lambert();

    return Fd + Fr;
}

