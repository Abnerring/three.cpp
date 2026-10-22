#version 450
// Port of _getEquirectMaterial in src/extras/PMREMGenerator.js. equirectUv is inlined
// from the common shader chunk rather than included.
layout(location = 0) in vec3 vWorldDirection;
layout(location = 0) out vec4 pc_fragColor;

layout(set = 0, binding = 1) uniform sampler2D envMap;

const float PI = 3.141592653589793;

vec2 equirectUv( vec3 dir ) {
    float u = atan( dir.z, dir.x ) / ( 2.0 * PI ) + 0.5;
    float v = asin( clamp( dir.y, - 1.0, 1.0 ) ) / PI + 0.5;
    return vec2( u, v );
}

void main() {
    // Average four subpixel samples to preserve small, bright features.
    vec3 direction = normalize( vWorldDirection );
    vec3 dx = dFdx( direction ) * 0.25;
    vec3 dy = dFdy( direction ) * 0.25;

    vec3 color = texture( envMap, equirectUv( normalize( direction - dx - dy ) ) ).rgb;
    color += texture( envMap, equirectUv( normalize( direction + dx - dy ) ) ).rgb;
    color += texture( envMap, equirectUv( normalize( direction - dx + dy ) ) ).rgb;
    color += texture( envMap, equirectUv( normalize( direction + dx + dy ) ) ).rgb;
    pc_fragColor = vec4( color * 0.25, 1.0 );
}
