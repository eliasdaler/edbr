#ifndef LIGHT_DATA_H
#define LIGHT_DATA_H

#define TYPE_DIRECTIONAL_LIGHT 0
#define TYPE_POINT_LIGHT 1
#define TYPE_SPOT_LIGHT 2

// layout in the SSBO
struct Light
{
    vec3 position;
    uint type;

    vec3 direction;
    float range;

    vec3 color;
    float intensity;

    vec2 scaleOffset; // spot light only
    uint shadowMapID;

    float unused;
};

#endif // LIGHT_DATA_H
