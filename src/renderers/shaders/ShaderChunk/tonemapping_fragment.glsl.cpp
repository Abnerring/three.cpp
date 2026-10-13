// Generated from three.js src/renderers/shaders/ShaderChunk/tonemapping_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/tonemapping_fragment.glsl.h"
namespace ShaderSource::Chunk::tonemapping_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
#if defined( TONE_MAPPING )

	gl_FragColor.rgb = toneMapping( gl_FragColor.rgb );

#endif
)three_glsl_0"; }
}
