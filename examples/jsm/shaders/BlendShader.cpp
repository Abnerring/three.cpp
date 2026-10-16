// Native port of the pinned examples/jsm/shaders/BlendShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/BlendShader.h"
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

namespace BlendShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view BlendShader_BlendShader_vertexShader = R"jsm1(

		varying vec2 vUv;

		void main() {

			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view BlendShader_BlendShader_fragmentShader = R"jsm2(

		uniform float opacity;
		uniform float mixRatio;

		uniform sampler2D tDiffuse1;
		uniform sampler2D tDiffuse2;

		varying vec2 vUv;

		void main() {

			vec4 texel1 = texture2D( tDiffuse1, vUv );
			vec4 texel2 = texture2D( tDiffuse2, vUv );
			gl_FragColor = opacity * mix( texel1, texel2, mixRatio );

		})jsm2";
} // namespace

JSValue nativeBlendShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "BlendShader");
    JSValue result = object({
        {"BlendShader", object({
            {"name", "BlendShader"},
            {"uniforms", object({
                {"tDiffuse1", object({
                    {"value", nullptr}
                })},
                {"tDiffuse2", object({
                    {"value", nullptr}
                })},
                {"mixRatio", object({
                    {"value", 0.5}
                })},
                {"opacity", object({
                    {"value", 1}
                })}
            })},
            {"vertexShader", std::string(BlendShader_BlendShader_vertexShader)},
            {"fragmentShader", std::string(BlendShader_BlendShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace BlendShader
