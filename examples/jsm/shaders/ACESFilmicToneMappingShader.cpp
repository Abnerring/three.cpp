// Native port of the pinned examples/jsm/shaders/ACESFilmicToneMappingShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/ACESFilmicToneMappingShader.h"
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

namespace {
JSValue object(std::initializer_list<std::pair<const char*,JSValue>> fields) {
    auto result=JSValue::object();
    for(const auto& [key,value]:fields){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;result.defineProperty(key,d);}
    return result;
}
JSValue array(std::initializer_list<JSValue> values) {
    auto result=JSValue::array();std::size_t index=0;
    for(const auto& value:values){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;result.defineProperty(std::to_string(index++),d);}
    return result;
}
} // namespace

namespace ACESFilmicToneMappingShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view ACESFilmicToneMappingShader_ACESFilmicToneMappingShader_vertexShader = R"jsm1(

		varying vec2 vUv;

		void main() {

			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view ACESFilmicToneMappingShader_ACESFilmicToneMappingShader_fragmentShader = R"jsm2(

		#define saturate(a) clamp( a, 0.0, 1.0 )

		uniform sampler2D tDiffuse;

		uniform float exposure;

		varying vec2 vUv;

		vec3 RRTAndODTFit( vec3 v ) {

			vec3 a = v * ( v + 0.0245786 ) - 0.000090537;
			vec3 b = v * ( 0.983729 * v + 0.4329510 ) + 0.238081;
			return a / b;

		}

		vec3 ACESFilmicToneMapping( vec3 color ) {

		// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
			const mat3 ACESInputMat = mat3(
				vec3( 0.59719, 0.07600, 0.02840 ), // transposed from source
				vec3( 0.35458, 0.90834, 0.13383 ),
				vec3( 0.04823, 0.01566, 0.83777 )
			);

		// ODT_SAT => XYZ => D60_2_D65 => sRGB
			const mat3 ACESOutputMat = mat3(
				vec3(  1.60475, -0.10208, -0.00327 ), // transposed from source
				vec3( -0.53108,  1.10813, -0.07276 ),
				vec3( -0.07367, -0.00605,  1.07602 )
			);

			color = ACESInputMat * color;

		// Apply RRT and ODT
			color = RRTAndODTFit( color );

			color = ACESOutputMat * color;

		// Clamp to [0, 1]
			return saturate( color );

		}

		void main() {

			vec4 tex = texture2D( tDiffuse, vUv );

			tex.rgb *= exposure / 0.6; // pre-exposed, outside of the tone mapping function

			gl_FragColor = vec4( ACESFilmicToneMapping( tex.rgb ), tex.a );

		})jsm2";
} // namespace

JSValue nativeACESFilmicToneMappingShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "ACESFilmicToneMappingShader");
    JSValue result = object({
        {"ACESFilmicToneMappingShader", object({
            {"name", "ACESFilmicToneMappingShader"},
            {"uniforms", object({
                {"tDiffuse", object({
                    {"value", nullptr}
                })},
                {"exposure", object({
                    {"value", 1}
                })}
            })},
            {"vertexShader", std::string(ACESFilmicToneMappingShader_ACESFilmicToneMappingShader_vertexShader)},
            {"fragmentShader", std::string(ACESFilmicToneMappingShader_ACESFilmicToneMappingShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace ACESFilmicToneMappingShader
