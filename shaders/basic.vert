#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in float viewDepth;
layout(location = 0) out vec4 vColor;
layout(location = 1) out float vFogDepth;
layout(push_constant) uniform Draw {
    mat4 mvp;
    vec4 diffuse;
    // x: alphaTest, y: opaque, z: premultiplied alpha, w: sRGB output
    vec4 settings;
    vec4 fogColor;
    // x: fog type (0, linear 1, exp2 2), y: near/density, z: far
    vec4 fogParams;
} draw;
void main() {
    gl_Position = draw.mvp * vec4(position, 1.0);
    vColor = vertexColor;
    vFogDepth = viewDepth;
}
