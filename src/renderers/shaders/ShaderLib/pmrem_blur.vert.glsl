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
    // sigma is only meaningful in the fragment stage, but it must be referenced here too:
    // Vulkan requires one push constant range to cover every stage that uses the block, and
    // a stage that omits the trailing float would reflect a shorter range and produce two
    // overlapping ranges, which is invalid.
    float sigmaGuard = transforms.sigma - transforms.sigma;
    vWorldDirection = normalize( ( transforms.modelMatrix * vec4( position, 0.0 ) ).xyz ) *
                      ( 1.0 + sigmaGuard );
    gl_Position = transforms.projectionMatrix * transforms.modelViewMatrix * vec4( position, 1.0 );
}
