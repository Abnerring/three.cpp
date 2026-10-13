// Generated from three.js src/renderers/shaders/ShaderChunk/map_particle_pars_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/map_particle_pars_fragment.glsl.h"
namespace ShaderSource::Chunk::map_particle_pars_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
#if defined( USE_POINTS_UV )

	varying vec2 vUv;

#else

	#if defined( USE_MAP ) || defined( USE_ALPHAMAP )

		uniform mat3 uvTransform;

	#endif

#endif

#ifdef USE_MAP

	uniform sampler2D map;

#endif

#ifdef USE_ALPHAMAP

	uniform sampler2D alphaMap;

#endif
)three_glsl_0"; }
}
