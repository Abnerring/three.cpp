// Generated from three.js src/renderers/shaders/ShaderChunk/clipping_planes_vertex.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/clipping_planes_vertex.glsl.h"
namespace ShaderSource::Chunk::clipping_planes_vertex {
std::string_view defaultSource() { return R"three_glsl_0(
#if NUM_CLIPPING_PLANES > 0

	vClipPosition = - mvPosition.xyz;

#endif
)three_glsl_0"; }
}
