#ifndef LIGHT_DATA_H
#define LIGHT_DATA_H

// layout in the SSBO
struct LightData
{
    vec4 positionAndType;
    vec4 directionAndRange;
    vec4 colorAndIntensity;
    vec4 scaleOffsetAndSMIndexAndUnused;
};

#endif // LIGHT_DATA_H
