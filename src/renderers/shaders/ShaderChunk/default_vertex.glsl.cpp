// Generated from three.js src/renderers/shaders/ShaderChunk/default_vertex.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/default_vertex.glsl.h"
namespace ShaderSource::Chunk::default_vertex {
std::string_view defaultSource() { return R"three_glsl_0(
void main() {
	gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );
}
)three_glsl_0"; }
}
