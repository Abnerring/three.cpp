// Generated from three.js src/renderers/shaders/ShaderChunk/metalnessmap_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/metalnessmap_fragment.glsl.h"
namespace ShaderSource::Chunk::metalnessmap_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
float metalnessFactor = metalness;

#ifdef USE_METALNESSMAP

	vec4 texelMetalness = texture2D( metalnessMap, vMetalnessMapUv );

	// reads channel B, compatible with a combined OcclusionRoughnessMetallic (RGB) texture
	metalnessFactor *= texelMetalness.b;

#endif
)three_glsl_0"; }
}
