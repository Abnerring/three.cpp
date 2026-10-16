// Native port of the pinned examples/jsm/shaders/ColorCorrectionShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/ColorCorrectionShader.h"
#include "core/NativeObject.h"
#include "math/Vector3.h"
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

namespace ColorCorrectionShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view ColorCorrectionShader_ColorCorrectionShader_vertexShader = R"jsm1(

		varying vec2 vUv;

		void main() {

			vUv = uv;

			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view ColorCorrectionShader_ColorCorrectionShader_fragmentShader = R"jsm2(

		uniform sampler2D tDiffuse;
		uniform vec3 powRGB;
		uniform vec3 mulRGB;
		uniform vec3 addRGB;

		varying vec2 vUv;

		void main() {

			gl_FragColor = texture2D( tDiffuse, vUv );
			gl_FragColor.rgb = mulRGB * pow( ( gl_FragColor.rgb + addRGB ), powRGB );

		})jsm2";
} // namespace

JSValue nativeColorCorrectionShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "ColorCorrectionShader");
    JSValue result = object({
        {"ColorCorrectionShader", object({
            {"name", "ColorCorrectionShader"},
            {"uniforms", object({
                {"tDiffuse", object({
                    {"value", nullptr}
                })},
                {"powRGB", object({
                    {"value", nativeVector(std::make_shared<Vector3>(2, 2, 2))}
                })},
                {"mulRGB", object({
                    {"value", nativeVector(std::make_shared<Vector3>(1, 1, 1))}
                })},
                {"addRGB", object({
                    {"value", nativeVector(std::make_shared<Vector3>(0, 0, 0))}
                })}
            })},
            {"vertexShader", std::string(ColorCorrectionShader_ColorCorrectionShader_vertexShader)},
            {"fragmentShader", std::string(ColorCorrectionShader_ColorCorrectionShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace ColorCorrectionShader
