// Generated from three.js src/renderers/shaders/ShaderChunk/alphatest_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/alphatest_fragment.glsl.h"
namespace ShaderSource::Chunk::alphatest_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
#ifdef USE_ALPHATEST

	#ifdef ALPHA_TO_COVERAGE

	diffuseColor.a = smoothstep( alphaTest, alphaTest + fwidth( diffuseColor.a ), diffuseColor.a );
	if ( diffuseColor.a == 0.0 ) discard;

	#else

	if ( diffuseColor.a < alphaTest ) discard;

	#endif

#endif
)three_glsl_0"; }
}
