// Native port of the pinned examples/jsm/shaders/LuminosityHighPassShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/LuminosityHighPassShader.h"
#include "core/NativeObject.h"
#include "math/Color.h"
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

namespace LuminosityHighPassShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view LuminosityHighPassShader_LuminosityHighPassShader_vertexShader = R"jsm1(

		varying vec2 vUv;

		void main() {

			vUv = uv;

			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view LuminosityHighPassShader_LuminosityHighPassShader_fragmentShader = R"jsm2(

		uniform sampler2D tDiffuse;
		uniform vec3 defaultColor;
		uniform float defaultOpacity;
		uniform float luminosityThreshold;
		uniform float smoothWidth;

		varying vec2 vUv;

		void main() {

			vec4 texel = texture2D( tDiffuse, vUv );

			float v = luminance( texel.xyz );

			vec4 outputColor = vec4( defaultColor.rgb, defaultOpacity );

			float alpha = smoothstep( luminosityThreshold, luminosityThreshold + smoothWidth, v );

			gl_FragColor = mix( outputColor, texel, alpha );

		})jsm2";
} // namespace

JSValue nativeLuminosityHighPassShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "LuminosityHighPassShader");
    JSValue result = object({
        {"LuminosityHighPassShader", object({
            {"name", "LuminosityHighPassShader"},
            {"uniforms", object({
                {"tDiffuse", object({
                    {"value", nullptr}
                })},
                {"luminosityThreshold", object({
                    {"value", 1}
                })},
                {"smoothWidth", object({
                    {"value", 1}
                })},
                {"defaultColor", object({
                    {"value", nativeColor(std::make_shared<Color>(0, 0, 0))}
                })},
                {"defaultOpacity", object({
                    {"value", 0}
                })}
            })},
            {"vertexShader", std::string(LuminosityHighPassShader_LuminosityHighPassShader_vertexShader)},
            {"fragmentShader", std::string(LuminosityHighPassShader_LuminosityHighPassShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace LuminosityHighPassShader
