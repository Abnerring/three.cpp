// Native port of the pinned examples/jsm/shaders/KaleidoShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/KaleidoShader.h"
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

namespace KaleidoShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view KaleidoShader_KaleidoShader_vertexShader = R"jsm1(

		varying vec2 vUv;

		void main() {

			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view KaleidoShader_KaleidoShader_fragmentShader = R"jsm2(

		uniform sampler2D tDiffuse;
		uniform float sides;
		uniform float angle;

		varying vec2 vUv;

		void main() {

			vec2 p = vUv - 0.5;
			float r = length(p);
			float a = atan(p.y, p.x) + angle;
			float tau = 2. * 3.1416 ;
			a = mod(a, tau/sides);
			a = abs(a - tau/sides/2.) ;
			p = r * vec2(cos(a), sin(a));
			vec4 color = texture2D(tDiffuse, p + 0.5);
			gl_FragColor = color;

		})jsm2";
} // namespace

JSValue nativeKaleidoShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "KaleidoShader");
    JSValue result = object({
        {"KaleidoShader", object({
            {"name", "KaleidoShader"},
            {"uniforms", object({
                {"tDiffuse", object({
                    {"value", nullptr}
                })},
                {"sides", object({
                    {"value", 6}
                })},
                {"angle", object({
                    {"value", 0}
                })}
            })},
            {"vertexShader", std::string(KaleidoShader_KaleidoShader_vertexShader)},
            {"fragmentShader", std::string(KaleidoShader_KaleidoShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace KaleidoShader
