#version 450
layout(set = 0, binding = 2, std140) uniform TextureTransforms {
    mat3 mapTransform;
    mat3 alphaTransform;
    mat4 modelView;
    uvec4 instance;
    uvec4 instanceLayout;
    uvec4 matrixDivisors;
    uvec4 inputLayout[4];
    uvec4 matrixEnabled;
} textures;
layout(set = 0, binding = 3, std430) readonly buffer InstanceMatrices { float values[]; } matrices;
layout(set = 0, binding = 4, std430) readonly buffer InstanceColors { float values[]; } colors;
layout(set = 0, binding = 5, std430) readonly buffer MorphTargets { vec4 values[]; } morphTargets;
layout(set = 0, binding = 6) uniform sampler2D morphWeights;
layout(set = 0, binding = 7, std430) readonly buffer Positions { float values[]; } positions;
layout(set = 0, binding = 8, std430) readonly buffer VertexColors { float values[]; } vertexColors;
layout(set = 0, binding = 9, std430) readonly buffer MapUVs { float values[]; } mapUVs;
layout(set = 0, binding = 10, std430) readonly buffer AlphaUVs { float values[]; } alphaUVs;
layout(location = 0) out vec4 vColor;
layout(location = 1) out float vFogDepth;
layout(location = 2) out vec2 vMapUv;
layout(location = 3) out vec2 vAlphaUv;
layout(location = 4) out vec3 vClipPosition;
layout(push_constant) uniform Draw {
    mat4 mvp;
    vec4 diffuse;
    // x: alphaTest, y: opaque, z: premultiplied alpha, w: sRGB output
    vec4 settings;
    vec4 fogColor;
    // x: fog type (0, linear 1, exp2 2), y: near/density, z: far
    vec4 fogParams;
} draw;
float morphWeight(ivec2 coordinate) {
    // WebGL 2 requires zero for out-of-range texel fetches.
    ivec2 extent = textureSize(morphWeights, 0);
    if (any(lessThan(coordinate, ivec2(0))) || any(greaterThanEqual(coordinate, extent)))
        return 0.0;
    return texelFetch(morphWeights, coordinate, 0).r;
}
uint inputOffset(uint slot) {
    uvec4 fetchLayout = textures.inputLayout[slot];
    uint row = fetchLayout.z == 0u ? 0u : fetchLayout.y == 0u ? uint(gl_VertexIndex) : uint(gl_InstanceIndex) / fetchLayout.y;
    return row * fetchLayout.x;
}
void main() {
    uint p = inputOffset(0u);
    vec3 transformed = vec3(positions.values[p],positions.values[p+1u],positions.values[p+2u]);
    if (textures.instanceLayout.z != 0u) {
        transformed *= morphWeight(ivec2(0, gl_InstanceIndex));
        for (uint i = 0u; i < textures.instanceLayout.z; ++i) {
            float influence = morphWeight(ivec2(i + 1u, gl_InstanceIndex));
            if (influence != 0.0)
                transformed += morphTargets.values[i * textures.instanceLayout.w + uint(gl_VertexIndex)].xyz * influence;
        }
    }
    vec4 localPosition = vec4(transformed, 1.0);
    if (textures.instance.x != 0u) {
        mat4 instanceMatrix;
        for (uint column = 0u; column < 4u; ++column) {
            uint divisor = textures.matrixDivisors[column];
            uint row = textures.matrixEnabled[column] == 0u ? 0u : divisor == 0u ? uint(gl_VertexIndex) : uint(gl_InstanceIndex) / divisor;
            uint offset = row * textures.instanceLayout.x;
            instanceMatrix[column] = vec4(matrices.values[offset + column * 4u], matrices.values[offset + column * 4u + 1u],
                matrices.values[offset + column * 4u + 2u], matrices.values[offset + column * 4u + 3u]);
        }
        localPosition = instanceMatrix * localPosition;
    }
    gl_Position = draw.mvp * localPosition;
    uint c = inputOffset(1u);
    vColor = vec4(vertexColors.values[c],vertexColors.values[c+1u],vertexColors.values[c+2u],
        textures.inputLayout[1].x == 4u ? vertexColors.values[c+3u] : 1.0);
    if (textures.instance.y != 0u) {
        uint row = textures.inputLayout[1].w == 0u ? 0u : textures.instance.w == 0u ? uint(gl_VertexIndex) : uint(gl_InstanceIndex) / textures.instance.w;
        uint offset = row * textures.instanceLayout.y;
        vColor.rgb *= vec3(colors.values[offset], colors.values[offset + 1u], colors.values[offset + 2u]);
    }
    vFogDepth = -(textures.modelView * localPosition).z;
    vClipPosition = -(textures.modelView * localPosition).xyz;
    uint m = inputOffset(2u);
    vMapUv = (textures.mapTransform * vec3(mapUVs.values[m],mapUVs.values[m+1u],1.0)).xy;
    uint a = inputOffset(3u);
    vAlphaUv = (textures.alphaTransform * vec3(alphaUVs.values[a],alphaUVs.values[a+1u],1.0)).xy;
}
