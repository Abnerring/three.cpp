// Native port of the pinned examples/jsm/shaders/DOFMipMapShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/DOFMipMapShader.h"
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

namespace DOFMipMapShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view DOFMipMapShader_DOFMipMapShader_vertexShader = R"jsm1(

		varying vec2 vUv;

		void main() {

			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view DOFMipMapShader_DOFMipMapShader_fragmentShader = R"jsm2(

		uniform float focus;
		uniform float maxblur;

		uniform sampler2D tColor;
		uniform sampler2D tDepth;

		varying vec2 vUv;

		void main() {

			vec4 depth = texture2D( tDepth, vUv );

			float factor = depth.x - focus;

			vec4 col = texture2D( tColor, vUv, 2.0 * maxblur * abs( focus - depth.x ) );

			gl_FragColor = col;
			gl_FragColor.a = 1.0;

		})jsm2";
} // namespace

JSValue nativeDOFMipMapShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "DOFMipMapShader");
    JSValue result = object({
        {"DOFMipMapShader", object({
            {"name", "DOFMipMapShader"},
            {"uniforms", object({
                {"tColor", object({
                    {"value", nullptr}
                })},
                {"tDepth", object({
                    {"value", nullptr}
                })},
                {"focus", object({
                    {"value", 1}
                })},
                {"maxblur", object({
                    {"value", 1}
                })}
            })},
            {"vertexShader", std::string(DOFMipMapShader_DOFMipMapShader_vertexShader)},
            {"fragmentShader", std::string(DOFMipMapShader_DOFMipMapShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace DOFMipMapShader
