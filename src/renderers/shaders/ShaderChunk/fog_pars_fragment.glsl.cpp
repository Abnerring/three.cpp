// Generated from three.js src/renderers/shaders/ShaderChunk/fog_pars_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/fog_pars_fragment.glsl.h"
namespace ShaderSource::Chunk::fog_pars_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
#ifdef USE_FOG

	uniform vec3 fogColor;
	varying float vFogDepth;

	#ifdef FOG_EXP2

		uniform float fogDensity;

	#else

		uniform float fogNear;
		uniform float fogFar;

	#endif

#endif
)three_glsl_0"; }
}
