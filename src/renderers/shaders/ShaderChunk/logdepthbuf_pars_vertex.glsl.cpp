// Generated from three.js src/renderers/shaders/ShaderChunk/logdepthbuf_pars_vertex.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/logdepthbuf_pars_vertex.glsl.h"
namespace ShaderSource::Chunk::logdepthbuf_pars_vertex {
std::string_view defaultSource() { return R"three_glsl_0(
#ifdef USE_LOGARITHMIC_DEPTH_BUFFER

	varying float vFragDepth;
	varying float vIsPerspective;

#endif
)three_glsl_0"; }
}
