// Generated from three.js src/renderers/shaders/ShaderChunk/beginnormal_vertex.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/beginnormal_vertex.glsl.h"
namespace ShaderSource::Chunk::beginnormal_vertex {
std::string_view defaultSource() { return R"three_glsl_0(
vec3 objectNormal = vec3( normal );

#ifdef USE_TANGENT

	vec3 objectTangent = vec3( tangent.xyz );

#endif
)three_glsl_0"; }
}
