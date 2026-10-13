// Generated from three.js src/renderers/shaders/ShaderChunk/roughnessmap_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/roughnessmap_fragment.glsl.h"
namespace ShaderSource::Chunk::roughnessmap_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
float roughnessFactor = roughness;

#ifdef USE_ROUGHNESSMAP

	vec4 texelRoughness = texture2D( roughnessMap, vRoughnessMapUv );

	// reads channel G, compatible with a combined OcclusionRoughnessMetallic (RGB) texture
	roughnessFactor *= texelRoughness.g;

#endif
)three_glsl_0"; }
}
