// Generated from three.js src/renderers/shaders/ShaderChunk/project_vertex.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/project_vertex.glsl.h"
namespace ShaderSource::Chunk::project_vertex {
std::string_view defaultSource() { return R"three_glsl_0(
vec4 mvPosition = vec4( transformed, 1.0 );

#ifdef USE_BATCHING

	mvPosition = batchingMatrix * mvPosition;

#endif

#ifdef USE_INSTANCING

	mvPosition = instanceMatrix * mvPosition;

#endif

mvPosition = modelViewMatrix * mvPosition;

gl_Position = projectionMatrix * mvPosition;
)three_glsl_0"; }
}
