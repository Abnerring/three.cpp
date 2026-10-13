// Generated from three.js src/renderers/shaders/ShaderChunk/normal_pars_vertex.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/normal_pars_vertex.glsl.h"
namespace ShaderSource::Chunk::normal_pars_vertex {
std::string_view defaultSource() { return R"three_glsl_0(
#ifndef FLAT_SHADED

	varying vec3 vNormal;

	#ifdef USE_TANGENT

		varying vec3 vTangent;
		varying vec3 vBitangent;

	#endif

#endif
)three_glsl_0"; }
}
