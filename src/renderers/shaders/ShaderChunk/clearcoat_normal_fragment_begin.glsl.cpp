// Generated from three.js src/renderers/shaders/ShaderChunk/clearcoat_normal_fragment_begin.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/clearcoat_normal_fragment_begin.glsl.h"
namespace ShaderSource::Chunk::clearcoat_normal_fragment_begin {
std::string_view defaultSource() { return R"three_glsl_0(
#ifdef USE_CLEARCOAT

	vec3 clearcoatNormal = nonPerturbedNormal;

#endif
)three_glsl_0"; }
}
