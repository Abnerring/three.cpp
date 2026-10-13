// Generated from three.js src/renderers/shaders/ShaderChunk/colorspace_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/colorspace_fragment.glsl.h"
namespace ShaderSource::Chunk::colorspace_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
gl_FragColor = linearToOutputTexel( gl_FragColor );
)three_glsl_0"; }
}
