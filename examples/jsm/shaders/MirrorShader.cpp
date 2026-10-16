// Native port of the pinned examples/jsm/shaders/MirrorShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/MirrorShader.h"
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

namespace MirrorShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view MirrorShader_MirrorShader_vertexShader = R"jsm1(

		varying vec2 vUv;

		void main() {

			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view MirrorShader_MirrorShader_fragmentShader = R"jsm2(

		uniform sampler2D tDiffuse;
		uniform int side;

		varying vec2 vUv;

		void main() {

			vec2 p = vUv;
			if (side == 0){
				if (p.x > 0.5) p.x = 1.0 - p.x;
			}else if (side == 1){
				if (p.x < 0.5) p.x = 1.0 - p.x;
			}else if (side == 2){
				if (p.y < 0.5) p.y = 1.0 - p.y;
			}else if (side == 3){
				if (p.y > 0.5) p.y = 1.0 - p.y;
			}
			vec4 color = texture2D(tDiffuse, p);
			gl_FragColor = color;

		})jsm2";
} // namespace

JSValue nativeMirrorShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "MirrorShader");
    JSValue result = object({
        {"MirrorShader", object({
            {"name", "MirrorShader"},
            {"uniforms", object({
                {"tDiffuse", object({
                    {"value", nullptr}
                })},
                {"side", object({
                    {"value", 1}
                })}
            })},
            {"vertexShader", std::string(MirrorShader_MirrorShader_vertexShader)},
            {"fragmentShader", std::string(MirrorShader_MirrorShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace MirrorShader
