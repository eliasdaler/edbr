#version 460

#extension GL_GOOGLE_include_directive : require

#include "scene_data.glsl"
#include "mesh_pcs.glsl"

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec4 outTangent;
layout (location = 4) out mat3 outTBN;

void main()
{
    Vertex v = pushConstants.vertexBuffer.vertices[gl_VertexIndex];

    vec4 worldPos = pushConstants.transform * vec4(v.position, 1.0f);

    gl_Position = sceneData.viewProj * worldPos;
    outPos = worldPos.xyz;
    outUV = vec2(v.uv_x, v.uv_y);
    // A bit inefficient, but okay - this is needed for non-uniform scale
    // models. See: http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/
    // Simpler case, when everything is uniform
    // outNormal = (pushConstants.transform * vec4(v.normal, 0.0)).xyz;
    outNormal = mat3(transpose(inverse(pushConstants.transform))) * v.normal;

    outTangent = v.tangent;

    vec3 T = normalize(vec3(pushConstants.transform * v.tangent));
    vec3 N = normalize(outNormal);
    vec3 B = cross(N, T) * v.tangent.w;
    outTBN = mat3(T, B, N);
}
