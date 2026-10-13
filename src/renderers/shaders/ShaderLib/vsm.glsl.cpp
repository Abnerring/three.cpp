// Generated from three.js src/renderers/shaders/ShaderLib/vsm.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderLib/vsm.glsl.h"
namespace ShaderSource::Lib::vsm {
std::string_view vertex() { return R"three_glsl_0(
void main() {

	gl_Position = vec4( position, 1.0 );

}
)three_glsl_0"; }
std::string_view fragment() { return R"three_glsl_0(
uniform sampler2D shadow_pass;
uniform vec2 resolution;
uniform float radius;

void main() {

	const float samples = float( VSM_SAMPLES );

	float mean = 0.0;
	float squared_mean = 0.0;

	float uvStride = samples <= 1.0 ? 0.0 : 2.0 / ( samples - 1.0 );
	float uvStart = samples <= 1.0 ? 0.0 : - 1.0;
	for ( float i = 0.0; i < samples; i ++ ) {

		float uvOffset = uvStart + i * uvStride;

		#ifdef HORIZONTAL_PASS

			vec2 distribution = texture2D( shadow_pass, ( gl_FragCoord.xy + vec2( uvOffset, 0.0 ) * radius ) / resolution ).rg;
			mean += distribution.x;
			squared_mean += distribution.y * distribution.y + distribution.x * distribution.x;

		#else

			float depth = texture2D( shadow_pass, ( gl_FragCoord.xy + vec2( 0.0, uvOffset ) * radius ) / resolution ).r;
			mean += depth;
			squared_mean += depth * depth;

		#endif

	}

	mean = mean / samples;
	squared_mean = squared_mean / samples;

	float std_dev = sqrt( max( 0.0, squared_mean - mean * mean ) );

	gl_FragColor = vec4( mean, std_dev, 0.0, 1.0 );

}
)three_glsl_0"; }
}
