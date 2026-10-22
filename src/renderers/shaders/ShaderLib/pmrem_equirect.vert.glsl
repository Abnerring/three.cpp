#version 450
// Mirrors ShaderChunk.cube_vert from the pinned revision: the world direction of the
// box vertex, which the fragment shader turns into an equirectangular lookup.
layout(location = 0) in vec3 position;
layout(location = 0) out vec3 vWorldDirection;

// Push constants rather than a uniform buffer: the three matrices are all this stage
// needs, and it keeps the pass free of descriptor bookkeeping.
layout(push_constant) uniform Transforms {
    mat4 modelMatrix;
    mat4 modelViewMatrix;
    mat4 projectionMatrix;
} transforms;

void main() {
    // transformDirection( position, modelMatrix )
    vWorldDirection = normalize( ( transforms.modelMatrix * vec4( position, 0.0 ) ).xyz );
    gl_Position = transforms.projectionMatrix * transforms.modelViewMatrix * vec4( position, 1.0 );
}
