#version 450
layout(location = 0) in vec4 vColor;
layout(location = 1) in float vFogDepth;
layout(location = 2) in vec2 vMapUv;
layout(location = 3) in vec2 vAlphaUv;
layout(location = 4) in vec3 vClipPosition;
layout(set = 0, binding = 0) uniform sampler2D mapTexture;
layout(set = 0, binding = 1) uniform sampler2D alphaTexture;
layout(set = 0, binding = 11, std430) readonly buffer ClippingPlanes {
    uvec4 counts;
    vec4 planes[];
} clipping;
layout(location = 0) out vec4 outColor;
layout(push_constant) uniform Draw {
    mat4 mvp;
    vec4 diffuse;
    vec4 settings;
    vec4 fogColor;
    vec4 fogParams;
} draw;
vec3 linearToSRGB(vec3 value) {
    return mix(pow(value, vec3(0.41666)) * 1.055 - vec3(0.055),
               value * 12.92, lessThanEqual(value, vec3(0.0031308)));
}
void main() {
    for (uint i = 0u; i < clipping.counts.y; ++i)
        if (dot(vClipPosition, clipping.planes[i].xyz) > clipping.planes[i].w) discard;
    if (clipping.counts.y < clipping.counts.x) {
        bool clipped = true;
        for (uint i = clipping.counts.y; i < clipping.counts.x; ++i)
            clipped = (dot(vClipPosition, clipping.planes[i].xyz) > clipping.planes[i].w) && clipped;
        if (clipped) discard;
    }
    vec4 color = draw.diffuse * texture(mapTexture, vMapUv);
    color *= vColor;
    color.a *= texture(alphaTexture, vAlphaUv).g;
    if (color.a < draw.settings.x) discard;
    if (draw.settings.y > 0.5) color.a = 1.0;
    if (draw.settings.w > 0.5) color.rgb = linearToSRGB(color.rgb);
    if (draw.fogParams.x > 0.5) {
        float fogFactor = draw.fogParams.x > 1.5
            ? 1.0 - exp(-draw.fogParams.y * draw.fogParams.y * vFogDepth * vFogDepth)
            : smoothstep(draw.fogParams.y, draw.fogParams.z, vFogDepth);
        color.rgb = mix(color.rgb, draw.fogColor.rgb, fogFactor);
    }
    if (draw.settings.z > 0.5) color.rgb *= color.a;
    outColor = color;
}
