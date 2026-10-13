// Generated from three.js src/renderers/shaders/ShaderChunk/clearcoat_normal_fragment_maps.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/clearcoat_normal_fragment_maps.glsl.h"
namespace ShaderSource::Chunk::clearcoat_normal_fragment_maps {
std::string_view defaultSource() { return R"three_glsl_0(
#ifdef USE_CLEARCOAT_NORMALMAP

	vec3 clearcoatMapN = texture2D( clearcoatNormalMap, vClearcoatNormalMapUv ).xyz * 2.0 - 1.0;
	clearcoatMapN.xy *= clearcoatNormalScale;

	clearcoatNormal = normalize( tbn2 * clearcoatMapN );

#endif
)three_glsl_0"; }
}
