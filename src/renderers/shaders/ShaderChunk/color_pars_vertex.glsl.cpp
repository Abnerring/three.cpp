// Generated from three.js src/renderers/shaders/ShaderChunk/color_pars_vertex.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/color_pars_vertex.glsl.h"
namespace ShaderSource::Chunk::color_pars_vertex {
std::string_view defaultSource() { return R"three_glsl_0(
#if defined( USE_COLOR ) || defined( USE_COLOR_ALPHA ) || defined( USE_INSTANCING_COLOR ) || defined( USE_BATCHING_COLOR )

	varying vec4 vColor;

#endif
)three_glsl_0"; }
}
