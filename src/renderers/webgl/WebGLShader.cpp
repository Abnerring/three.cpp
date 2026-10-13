// Direct port of three.js WebGLShader.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See THREEJS-LICENSE.
#include "renderers/webgl/WebGLShader.h"
namespace {
using Args = std::vector<JSValue>;
JSValue argument(const Args& args, std::size_t index) {
    return index < args.size() ? args[index] : JSValue();
}
JSValue invoke(const JSValue&, const Args& args, const Args&) {
    const auto gl = argument(args, 0), type = argument(args, 1), source = argument(args, 2);
    const auto shader = gl.get("createShader").call(gl, {type});
    gl.get("shaderSource").call(gl, {shader, source});
    gl.get("compileShader").call(gl, {shader});
    return shader;
}
} // namespace
JSValue nativeWebGLShaderConstructor() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isCallable())
        return *existing;
    auto result =
        JSValue::constructor(invoke, {}, "WebGLShader", 3, JSValue::ConstructorKind::Callable);
    result.retainForHeapLifetime();
    cache = JSWeakValue(result);
    return result;
}
JSValue WebGLShader(const JSValue& gl, const JSValue& type, const JSValue& source) {
    static_cast<void>(JSWeakValue(gl));
    static_cast<void>(JSWeakValue(type));
    static_cast<void>(JSWeakValue(source));
    return nativeWebGLShaderConstructor().call({}, {gl, type, source});
}
