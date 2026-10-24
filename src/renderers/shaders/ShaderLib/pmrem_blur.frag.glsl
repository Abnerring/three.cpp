#version 450
// Port of _getBlurMaterial in src/extras/PMREMGenerator.js: a spherical Gaussian blur
// sampled on a golden angle spiral, truncated at three standard deviations.
layout(location = 0) in vec3 vWorldDirection;
layout(location = 0) out vec4 pc_fragColor;

layout(set = 0, binding = 0) uniform samplerCube envMap;

layout(push_constant) uniform Transforms {
    mat4 modelMatrix;
    mat4 modelViewMatrix;
    mat4 projectionMatrix;
    float sigma;
} transforms;

const float PI = 3.141592653589793;
const int SAMPLES = 20;
const float GOLDEN_ANGLE = 2.39996322973;

void main() {
    vec3 outputDirection = normalize( vWorldDirection );

    vec3 up = abs( outputDirection.z ) < 0.999 ? vec3( 0.0, 0.0, 1.0 ) : vec3( 1.0, 0.0, 0.0 );
    vec3 tangent = normalize( cross( up, outputDirection ) );
    vec3 bitangent = cross( outputDirection, tangent );

    // Truncate the kernel at three standard deviations or at the antipode.
    float thetaMax = min( 3.0 * transforms.sigma, PI );
    float truncation = 1.0 - exp( - 0.5 * thetaMax * thetaMax / ( transforms.sigma * transforms.sigma ) );

    vec3 accumColor = vec3( 0.0 );
    float accumWeight = 0.0;

    for ( int i = 0; i < SAMPLES; i ++ ) {
        // Stratified inverse-CDF sampling of the Gaussian, placed on a golden angle spiral.
        float stratum = ( float( i ) + 0.5 ) / float( SAMPLES );
        float theta = transforms.sigma * sqrt( - 2.0 * log( 1.0 - stratum * truncation ) );
        float phi = float( i ) * GOLDEN_ANGLE;

        vec3 offset = cos( phi ) * tangent + sin( phi ) * bitangent;
        vec3 sampleDirection = cos( theta ) * outputDirection + sin( theta ) * offset;

        // Correct the planar sample density to solid angle.
        float weight = sin( theta ) / theta;

        accumColor += weight * textureLod( envMap, sampleDirection, 0.0 ).rgb;
        accumWeight += weight;
    }

    pc_fragColor = vec4( accumColor / accumWeight, 1.0 );
}
