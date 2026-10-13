// Generated from three.js src/renderers/shaders/ShaderChunk/opaque_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/opaque_fragment.glsl.h"
namespace ShaderSource::Chunk::opaque_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
#ifdef OPAQUE
diffuseColor.a = 1.0;
#endif

#ifdef USE_TRANSMISSION
diffuseColor.a *= material.transmissionAlpha;
#endif

gl_FragColor = vec4( outgoingLight, diffuseColor.a );
)three_glsl_0"; }
}
