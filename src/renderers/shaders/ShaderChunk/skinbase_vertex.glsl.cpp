// Generated from three.js src/renderers/shaders/ShaderChunk/skinbase_vertex.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/skinbase_vertex.glsl.h"
namespace ShaderSource::Chunk::skinbase_vertex {
std::string_view defaultSource() { return R"three_glsl_0(
#ifdef USE_SKINNING

	mat4 boneMatX = getBoneMatrix( skinIndex.x );
	mat4 boneMatY = getBoneMatrix( skinIndex.y );
	mat4 boneMatZ = getBoneMatrix( skinIndex.z );
	mat4 boneMatW = getBoneMatrix( skinIndex.w );

#endif
)three_glsl_0"; }
}
