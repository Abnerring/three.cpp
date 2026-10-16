// Native port of the pinned examples/jsm/shaders/TechnicolorShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/TechnicolorShader.h"
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

namespace TechnicolorShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view TechnicolorShader_TechnicolorShader_vertexShader = R"jsm1(

		varying vec2 vUv;

		void main() {

			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view TechnicolorShader_TechnicolorShader_fragmentShader = R"jsm2(

		uniform sampler2D tDiffuse;
		varying vec2 vUv;

		void main() {

			vec4 tex = texture2D( tDiffuse, vec2( vUv.x, vUv.y ) );
			vec4 newTex = vec4(tex.r, (tex.g + tex.b) * .5, (tex.g + tex.b) * .5, 1.0);

			gl_FragColor = newTex;

		})jsm2";
} // namespace

JSValue nativeTechnicolorShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "TechnicolorShader");
    JSValue result = object({
        {"TechnicolorShader", object({
            {"name", "TechnicolorShader"},
            {"uniforms", object({
                {"tDiffuse", object({
                    {"value", nullptr}
                })}
            })},
            {"vertexShader", std::string(TechnicolorShader_TechnicolorShader_vertexShader)},
            {"fragmentShader", std::string(TechnicolorShader_TechnicolorShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace TechnicolorShader
