// Generated from three.js src/renderers/shaders/ShaderChunk/fog_vertex.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/fog_vertex.glsl.h"
namespace ShaderSource::Chunk::fog_vertex {
std::string_view defaultSource() { return R"three_glsl_0(
#ifdef USE_FOG

	vFogDepth = - mvPosition.z;

#endif
)three_glsl_0"; }
}
