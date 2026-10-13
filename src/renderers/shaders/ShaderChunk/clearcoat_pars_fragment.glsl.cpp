// Generated from three.js src/renderers/shaders/ShaderChunk/clearcoat_pars_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/clearcoat_pars_fragment.glsl.h"
namespace ShaderSource::Chunk::clearcoat_pars_fragment {
std::string_view defaultSource() { return R"three_glsl_0(

#ifdef USE_CLEARCOATMAP

	uniform sampler2D clearcoatMap;

#endif

#ifdef USE_CLEARCOAT_NORMALMAP

	uniform sampler2D clearcoatNormalMap;
	uniform vec2 clearcoatNormalScale;

#endif

#ifdef USE_CLEARCOAT_ROUGHNESSMAP

	uniform sampler2D clearcoatRoughnessMap;

#endif
)three_glsl_0"; }
}
