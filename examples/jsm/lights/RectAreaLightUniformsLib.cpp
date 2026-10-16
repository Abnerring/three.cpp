// Native port of the pinned examples/jsm/lights/RectAreaLightUniformsLib.js (see upstream.json).
//
// Statement-by-statement port of the static init(): the four textures are read out of the
// (freshly re-initialized) RectAreaLightTexturesLib class object and assigned to UniformsLib
// in the upstream order, so the identity relations and the observable property writes are
// the same as in the pinned module.
#include "lights/RectAreaLightUniformsLib.h"
#include "lights/RectAreaLightTexturesLib.h"
#include "renderers/shaders/UniformsLib.h"
#include <utility>
#include <vector>

namespace RectAreaLightUniformsLib {
namespace {
using Args = std::vector<JSValue>;

// `new RectAreaLightUniformsLib()`: the upstream class declares no constructor body, so the
// fresh instance is returned unchanged.
JSValue constructClass(const JSValue&, const Args&, const Args&) { return {}; }
} // namespace

JSValue init() {
    // RectAreaLightTexturesLib.init();
    const auto texturesLib = RectAreaLightTexturesLib::rectAreaLightTexturesLib();
    texturesLib.get("init").call(texturesLib);

    // const { LTC_FLOAT_1, LTC_FLOAT_2, LTC_HALF_1, LTC_HALF_2 } = RectAreaLightTexturesLib;
    const auto LTC_FLOAT_1 = texturesLib.get("LTC_FLOAT_1");
    const auto LTC_FLOAT_2 = texturesLib.get("LTC_FLOAT_2");
    const auto LTC_HALF_1 = texturesLib.get("LTC_HALF_1");
    const auto LTC_HALF_2 = texturesLib.get("LTC_HALF_2");

    // UniformsLib.LTC_FLOAT_1 = LTC_FLOAT_1; ... (the four assignments, in upstream order)
    auto uniformsLib = nativeUniformsLib();
    uniformsLib.set("LTC_FLOAT_1", LTC_FLOAT_1);
    uniformsLib.set("LTC_FLOAT_2", LTC_FLOAT_2);
    uniformsLib.set("LTC_HALF_1", LTC_HALF_1);
    uniformsLib.set("LTC_HALF_2", LTC_HALF_2);

    return {};
}

JSValue init(const JSValue&, const Args&, const Args&) { return init(); }

JSValue rectAreaLightUniformsLib() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "RectAreaLightUniformsLib");

    // `class RectAreaLightUniformsLib { static init() {} }` has no static fields, so its own
    // properties are exactly the function metadata plus the non-enumerable `init` method.
    auto result = JSValue::constructor(
        constructClass, {}, "RectAreaLightUniformsLib", 0, JSValue::ConstructorKind::Class);
    JSPropertyDescriptor method;
    method.value = JSValue::function(init, {}, "init", 0);
    method.writable = method.configurable = true;
    method.enumerable = false;
    result.defineProperty("init", method);

    result.retainForHeapLifetime();
    cache = JSWeakValue(result);
    return result;
}

JSValue nativeRectAreaLightUniformsLib() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "RectAreaLightUniformsLib");

    auto result = JSValue::object();
    JSPropertyDescriptor descriptor;
    descriptor.value = rectAreaLightUniformsLib();
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    result.defineProperty("RectAreaLightUniformsLib", descriptor);

    result.retainForHeapLifetime();
    cache = JSWeakValue(result);
    return result;
}

} // namespace RectAreaLightUniformsLib
