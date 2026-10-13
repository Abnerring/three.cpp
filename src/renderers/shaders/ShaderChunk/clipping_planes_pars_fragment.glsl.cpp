// Generated from three.js src/renderers/shaders/ShaderChunk/clipping_planes_pars_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/clipping_planes_pars_fragment.glsl.h"
namespace ShaderSource::Chunk::clipping_planes_pars_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
#if NUM_CLIPPING_PLANES > 0

	varying vec3 vClipPosition;

	uniform vec4 clippingPlanes[ NUM_CLIPPING_PLANES ];

#endif
)three_glsl_0"; }
}
