// Native port of the pinned examples/jsm/shaders/NormalMapShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/NormalMapShader.h"
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

namespace NormalMapShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view NormalMapShader_NormalMapShader_vertexShader = R"jsm1(

		varying vec2 vUv;

		void main() {

			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view NormalMapShader_NormalMapShader_fragmentShader = R"jsm2(

		uniform float height;
		uniform vec2 resolution;
		uniform sampler2D heightMap;

		varying vec2 vUv;

		void main() {

			float val = texture2D( heightMap, vUv ).x;

			float valU = texture2D( heightMap, vUv + vec2( 1.0 / resolution.x, 0.0 ) ).x;
			float valV = texture2D( heightMap, vUv + vec2( 0.0, 1.0 / resolution.y ) ).x;

			gl_FragColor = vec4( ( 0.5 * normalize( vec3( val - valU, val - valV, height  ) ) + 0.5 ), 1.0 );

		})jsm2";
} // namespace

JSValue nativeNormalMapShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "NormalMapShader");
    JSValue result = object({
        {"NormalMapShader", object({
            {"name", "NormalMapShader"},
            {"uniforms", object({
                {"heightMap", object({
                    {"value", nullptr}
                })},
                {"resolution", object({
                    {"value", nativeVector(std::make_shared<Vector2>(512, 512))}
                })},
                {"scale", object({
                    {"value", nativeVector(std::make_shared<Vector2>(1, 1))}
                })},
                {"height", object({
                    {"value", 0.05}
                })}
            })},
            {"vertexShader", std::string(NormalMapShader_NormalMapShader_vertexShader)},
            {"fragmentShader", std::string(NormalMapShader_NormalMapShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace NormalMapShader
