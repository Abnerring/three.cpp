// Native port of the pinned examples/jsm/shaders/TriangleBlurShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/TriangleBlurShader.h"
#include "core/NativeObject.h"
#include "math/Vector2.h"
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

namespace TriangleBlurShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view TriangleBlurShader_TriangleBlurShader_vertexShader = R"jsm1(

		varying vec2 vUv;

		void main() {

			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view TriangleBlurShader_TriangleBlurShader_fragmentShader = R"jsm2(

		#include <common>

		#define ITERATIONS 10.0

		uniform sampler2D texture;
		uniform vec2 delta;

		varying vec2 vUv;

		void main() {

			vec4 color = vec4( 0.0 );

			float total = 0.0;

		// randomize the lookup values to hide the fixed number of samples

			float offset = rand( vUv );

			for ( float t = -ITERATIONS; t <= ITERATIONS; t ++ ) {

				float percent = ( t + offset - 0.5 ) / ITERATIONS;
				float weight = 1.0 - abs( percent );

				color += texture2D( texture, vUv + delta * percent ) * weight;
				total += weight;

			}

			gl_FragColor = color / total;

		})jsm2";
} // namespace

JSValue nativeTriangleBlurShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "TriangleBlurShader");
    JSValue result = object({
        {"TriangleBlurShader", object({
            {"name", "TriangleBlurShader"},
            {"uniforms", object({
                {"texture", object({
                    {"value", nullptr}
                })},
                {"delta", object({
                    {"value", nativeVector(std::make_shared<Vector2>(1, 1))}
                })}
            })},
            {"vertexShader", std::string(TriangleBlurShader_TriangleBlurShader_vertexShader)},
            {"fragmentShader", std::string(TriangleBlurShader_TriangleBlurShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace TriangleBlurShader
