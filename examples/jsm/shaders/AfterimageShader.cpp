// Native port of the pinned examples/jsm/shaders/AfterimageShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/AfterimageShader.h"
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

namespace AfterimageShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view AfterimageShader_AfterimageShader_vertexShader = R"jsm1(

		varying vec2 vUv;

		void main() {

			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view AfterimageShader_AfterimageShader_fragmentShader = R"jsm2(

		uniform float damp;

		uniform sampler2D tOld;
		uniform sampler2D tNew;

		varying vec2 vUv;

		vec4 when_gt( vec4 x, float y ) {

			return max( sign( x - y ), 0.0 );

		}

		void main() {

			vec4 texelOld = texture2D( tOld, vUv );
			vec4 texelNew = texture2D( tNew, vUv );

			texelOld *= damp * when_gt( texelOld, 0.1 );

			gl_FragColor = max(texelNew, texelOld);

		})jsm2";
} // namespace

JSValue nativeAfterimageShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "AfterimageShader");
    JSValue result = object({
        {"AfterimageShader", object({
            {"name", "AfterimageShader"},
            {"uniforms", object({
                {"damp", object({
                    {"value", 0.96}
                })},
                {"tOld", object({
                    {"value", nullptr}
                })},
                {"tNew", object({
                    {"value", nullptr}
                })}
            })},
            {"vertexShader", std::string(AfterimageShader_AfterimageShader_vertexShader)},
            {"fragmentShader", std::string(AfterimageShader_AfterimageShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace AfterimageShader
