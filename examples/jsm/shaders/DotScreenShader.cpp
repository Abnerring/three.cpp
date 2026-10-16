// Native port of the pinned examples/jsm/shaders/DotScreenShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/DotScreenShader.h"
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

namespace DotScreenShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view DotScreenShader_DotScreenShader_vertexShader = R"jsm1(

		varying vec2 vUv;

		void main() {

			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view DotScreenShader_DotScreenShader_fragmentShader = R"jsm2(

		uniform vec2 center;
		uniform float angle;
		uniform float scale;
		uniform vec2 tSize;

		uniform sampler2D tDiffuse;

		varying vec2 vUv;

		float pattern() {

			float s = sin( angle ), c = cos( angle );

			vec2 tex = vUv * tSize - center;
			vec2 point = vec2( c * tex.x - s * tex.y, s * tex.x + c * tex.y ) * scale;

			return ( sin( point.x ) * sin( point.y ) ) * 4.0;

		}

		void main() {

			vec4 color = texture2D( tDiffuse, vUv );

			float average = ( color.r + color.g + color.b ) / 3.0;

			gl_FragColor = vec4( vec3( average * 10.0 - 5.0 + pattern() ), color.a );

		})jsm2";
} // namespace

JSValue nativeDotScreenShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "DotScreenShader");
    JSValue result = object({
        {"DotScreenShader", object({
            {"name", "DotScreenShader"},
            {"uniforms", object({
                {"tDiffuse", object({
                    {"value", nullptr}
                })},
                {"tSize", object({
                    {"value", nativeVector(std::make_shared<Vector2>(256, 256))}
                })},
                {"center", object({
                    {"value", nativeVector(std::make_shared<Vector2>(0.5, 0.5))}
                })},
                {"angle", object({
                    {"value", 1.57}
                })},
                {"scale", object({
                    {"value", 1}
                })}
            })},
            {"vertexShader", std::string(DotScreenShader_DotScreenShader_vertexShader)},
            {"fragmentShader", std::string(DotScreenShader_DotScreenShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace DotScreenShader
