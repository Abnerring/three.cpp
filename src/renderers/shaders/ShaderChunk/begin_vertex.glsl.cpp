// Generated from three.js src/renderers/shaders/ShaderChunk/begin_vertex.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/begin_vertex.glsl.h"
namespace ShaderSource::Chunk::begin_vertex {
std::string_view defaultSource() { return R"three_glsl_0(
vec3 transformed = vec3( position );

#ifdef USE_ALPHAHASH

	vPosition = vec3( position );

#endif
)three_glsl_0"; }
}
