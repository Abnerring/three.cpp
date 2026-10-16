// Native port of the pinned examples/jsm/shaders/BrightnessContrastShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/BrightnessContrastShader.h"
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

namespace BrightnessContrastShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view BrightnessContrastShader_BrightnessContrastShader_vertexShader = R"jsm1(

		varying vec2 vUv;

		void main() {

			vUv = uv;

			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view BrightnessContrastShader_BrightnessContrastShader_fragmentShader = R"jsm2(

		uniform sampler2D tDiffuse;
		uniform float brightness;
		uniform float contrast;

		varying vec2 vUv;

		void main() {

			gl_FragColor = texture2D( tDiffuse, vUv );

			gl_FragColor.rgb += brightness;

			if (contrast > 0.0) {
				gl_FragColor.rgb = (gl_FragColor.rgb - 0.5) / (1.0 - contrast) + 0.5;
			} else {
				gl_FragColor.rgb = (gl_FragColor.rgb - 0.5) * (1.0 + contrast) + 0.5;
			}

		})jsm2";
} // namespace

JSValue nativeBrightnessContrastShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "BrightnessContrastShader");
    JSValue result = object({
        {"BrightnessContrastShader", object({
            {"name", "BrightnessContrastShader"},
            {"uniforms", object({
                {"tDiffuse", object({
                    {"value", nullptr}
                })},
                {"brightness", object({
                    {"value", 0}
                })},
                {"contrast", object({
                    {"value", 0}
                })}
            })},
            {"vertexShader", std::string(BrightnessContrastShader_BrightnessContrastShader_vertexShader)},
            {"fragmentShader", std::string(BrightnessContrastShader_BrightnessContrastShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace BrightnessContrastShader
