// Generated from three.js src/renderers/shaders/ShaderChunk/displacementmap_vertex.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/displacementmap_vertex.glsl.h"
namespace ShaderSource::Chunk::displacementmap_vertex {
std::string_view defaultSource() { return R"three_glsl_0(
#ifdef USE_DISPLACEMENTMAP

	transformed += normalize( objectNormal ) * ( texture2D( displacementMap, vDisplacementMapUv ).x * displacementScale + displacementBias );

#endif
)three_glsl_0"; }
}
