// Generated from three.js src/renderers/shaders/ShaderChunk/alphamap_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/alphamap_fragment.glsl.h"
namespace ShaderSource::Chunk::alphamap_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
#ifdef USE_ALPHAMAP

	diffuseColor.a *= texture2D( alphaMap, vAlphaMapUv ).g;

#endif
)three_glsl_0"; }
}
