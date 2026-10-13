// Generated from three.js src/renderers/shaders/ShaderChunk/dithering_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/dithering_fragment.glsl.h"
namespace ShaderSource::Chunk::dithering_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
#ifdef DITHERING

	gl_FragColor.rgb = dithering( gl_FragColor.rgb );

#endif
)three_glsl_0"; }
}
