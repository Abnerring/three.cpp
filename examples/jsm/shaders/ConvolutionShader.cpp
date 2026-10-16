// Native port of the pinned examples/jsm/shaders/ConvolutionShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/ConvolutionShader.h"
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

namespace ConvolutionShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view ConvolutionShader_ConvolutionShader_vertexShader = R"jsm1(

		uniform vec2 uImageIncrement;

		varying vec2 vUv;

		void main() {

			vUv = uv - ( ( KERNEL_SIZE_FLOAT - 1.0 ) / 2.0 ) * uImageIncrement;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view ConvolutionShader_ConvolutionShader_fragmentShader = R"jsm2(

		uniform float cKernel[ KERNEL_SIZE_INT ];

		uniform sampler2D tDiffuse;
		uniform vec2 uImageIncrement;

		varying vec2 vUv;

		void main() {

			vec2 imageCoord = vUv;
			vec4 sum = vec4( 0.0, 0.0, 0.0, 0.0 );

			for( int i = 0; i < KERNEL_SIZE_INT; i ++ ) {

				sum += texture2D( tDiffuse, imageCoord ) * cKernel[ i ];
				imageCoord += uImageIncrement;

			}

			gl_FragColor = sum;

		})jsm2";
} // namespace

JSValue nativeConvolutionShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "ConvolutionShader");
    JSValue result = object({
        {"ConvolutionShader", object({
            {"name", "ConvolutionShader"},
            {"defines", object({
                {"KERNEL_SIZE_FLOAT", "25.0"},
                {"KERNEL_SIZE_INT", "25"}
            })},
            {"uniforms", object({
                {"tDiffuse", object({
                    {"value", nullptr}
                })},
                {"uImageIncrement", object({
                    {"value", nativeVector(std::make_shared<Vector2>(0.001953125, 0))}
                })},
                {"cKernel", object({
                    {"value", array({})}
                })}
            })},
            {"vertexShader", std::string(ConvolutionShader_ConvolutionShader_vertexShader)},
            {"fragmentShader", std::string(ConvolutionShader_ConvolutionShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace ConvolutionShader
