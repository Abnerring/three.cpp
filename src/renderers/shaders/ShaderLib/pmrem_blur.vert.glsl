#version 450
// Same as the equirectangular stage: the world direction of the box vertex.
layout(location = 0) in vec3 position;
layout(location = 0) out vec3 vWorldDirection;

layout(push_constant) uniform Transforms {
    mat4 modelMatrix;
    mat4 modelViewMatrix;
    mat4 projectionMatrix;
    float sigma;
} transforms;

void main() {
    vWorldDirection = normalize( ( transforms.modelMatrix * vec4( position, 0.0 ) ).xyz );
    gl_Position = transforms.projectionMatrix * transforms.modelViewMatrix * vec4( position, 1.0 );
}
