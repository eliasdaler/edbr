#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_scalar_block_layout: require

#include "vertex.glsl"
#include "scene_data.glsl"

layout (buffer_reference, scalar) readonly buffer PointLightVPsBuffer {
	mat4 vps[];
};

layout (push_constant) uniform constants
{
    mat4 model;
    VertexBuffer vertexBuffer;
    SceneDataBuffer sceneData;
    PointLightVPsBuffer vpsBuffer;
    uint materialID;
    uint lightIndex;
    uint vpsBufferIndex; // index in vpsBuffer array
    float farPlane;
} pcs;

