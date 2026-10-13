// Generated from three.js src/renderers/shaders/ShaderChunk/color_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/color_fragment.glsl.h"
namespace ShaderSource::Chunk::color_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
#if defined( USE_COLOR ) || defined( USE_COLOR_ALPHA )

	diffuseColor *= vColor;

#endif
)three_glsl_0"; }
}
