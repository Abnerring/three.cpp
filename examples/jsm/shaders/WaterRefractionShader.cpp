// Native port of the pinned examples/jsm/shaders/WaterRefractionShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/WaterRefractionShader.h"
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

namespace WaterRefractionShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view WaterRefractionShader_WaterRefractionShader_vertexShader = R"jsm1(

		uniform mat4 textureMatrix;

		varying vec2 vUv;
		varying vec4 vUvRefraction;

		void main() {

			vUv = uv;

			vUvRefraction = textureMatrix * vec4( position, 1.0 );

			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view WaterRefractionShader_WaterRefractionShader_fragmentShader = R"jsm2(

		uniform vec3 color;
		uniform float time;
		uniform sampler2D tDiffuse;
		uniform sampler2D tDudv;

		varying vec2 vUv;
		varying vec4 vUvRefraction;

		float blendOverlay( float base, float blend ) {

			return( base < 0.5 ? ( 2.0 * base * blend ) : ( 1.0 - 2.0 * ( 1.0 - base ) * ( 1.0 - blend ) ) );

		}

		vec3 blendOverlay( vec3 base, vec3 blend ) {

			return vec3( blendOverlay( base.r, blend.r ), blendOverlay( base.g, blend.g ),blendOverlay( base.b, blend.b ) );

		}

		void main() {

		 float waveStrength = 0.5;
		 float waveSpeed = 0.03;

			// simple distortion (ripple) via dudv map (see https://www.youtube.com/watch?v=6B7IF6GOu7s)

			vec2 distortedUv = texture2D( tDudv, vec2( vUv.x + time * waveSpeed, vUv.y ) ).rg * waveStrength;
			distortedUv = vUv.xy + vec2( distortedUv.x, distortedUv.y + time * waveSpeed );
			vec2 distortion = ( texture2D( tDudv, distortedUv ).rg * 2.0 - 1.0 ) * waveStrength;

			// new uv coords

			vec4 uv = vec4( vUvRefraction );
			uv.xy += distortion;

			vec4 base = texture2DProj( tDiffuse, uv );

			gl_FragColor = vec4( blendOverlay( base.rgb, color ), 1.0 );

			#include <tonemapping_fragment>
			#include <colorspace_fragment>

		})jsm2";
} // namespace

JSValue nativeWaterRefractionShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "WaterRefractionShader");
    JSValue result = object({
        {"WaterRefractionShader", object({
            {"name", "WaterRefractionShader"},
            {"uniforms", object({
                {"color", object({
                    {"value", nullptr}
                })},
                {"time", object({
                    {"value", 0}
                })},
                {"tDiffuse", object({
                    {"value", nullptr}
                })},
                {"tDudv", object({
                    {"value", nullptr}
                })},
                {"textureMatrix", object({
                    {"value", nullptr}
                })}
            })},
            {"vertexShader", std::string(WaterRefractionShader_WaterRefractionShader_vertexShader)},
            {"fragmentShader", std::string(WaterRefractionShader_WaterRefractionShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace WaterRefractionShader
