// Generated from three.js src/renderers/shaders/ShaderChunk/logdepthbuf_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/logdepthbuf_fragment.glsl.h"
namespace ShaderSource::Chunk::logdepthbuf_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
#if defined( USE_LOGARITHMIC_DEPTH_BUFFER )

	// Doing a strict comparison with == 1.0 can cause noise artifacts
	// on some platforms. See issue #17623.
	gl_FragDepth = vIsPerspective == 0.0 ? gl_FragCoord.z : log2( vFragDepth ) * logDepthBufFC * 0.5;

#endif
)three_glsl_0"; }
}
