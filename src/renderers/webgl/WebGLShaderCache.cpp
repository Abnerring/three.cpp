// Direct port of three.js WebGLShaderCache.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See THREEJS-LICENSE.
#include "renderers/webgl/WebGLShaderCache.h"
#include "core/JSIterator.h"
#include "core/JSMap.h"
#include "core/JSSet.h"

namespace {
using Args = std::vector<JSValue>;
JSValue argument(const Args& args, std::size_t index) {
    return index < args.size() ? args[index] : JSValue{};
}
JSValue call(const JSValue& receiver, const char* name, const Args& args = {}) {
    return receiver.get(name).call(receiver, args);
}
void method(JSValue prototype, const char* name, JSValue::NativeFunction callback,
            unsigned arity, const Args& captures = {}) {
    JSPropertyDescriptor descriptor;
    descriptor.value = JSValue::function(callback, captures, name, arity);
    descriptor.writable = descriptor.configurable = true;
    descriptor.enumerable = false;
    prototype.defineProperty(name, descriptor);
}
JSValue initializeStage(const JSValue& self, const Args& args, const Args& captures) {
    auto state = captures[0], target = self;
    const auto id = state.get("id").toNumber();
    state.set("id", id + 1);
    target.set("id", id);
    target.set("code", argument(args, 0));
    target.set("usedTimes", 0);
    return {};
}
JSValue initializeCache(const JSValue& self, const Args&, const Args&) {
    auto target = self;
    target.set("shaderCache", createNativeMap());
    target.set("materialCache", createNativeMap());
    return {};
}
JSValue update(const JSValue& self, const Args& args, const Args&) {
    const auto material = argument(args, 0), vertex = argument(args, 1), fragment = argument(args, 2);
    const auto shaders = call(self, "_getShaderCacheForMaterial", {material});
    if (call(shaders, "has", {vertex}).strictEquals(false)) {
        call(shaders, "add", {vertex});
        auto target = vertex;
        const auto count = target.get("usedTimes").toNumber();
        target.set("usedTimes", count + 1);
    }
    if (call(shaders, "has", {fragment}).strictEquals(false)) {
        call(shaders, "add", {fragment});
        auto target = fragment;
        const auto count = target.get("usedTimes").toNumber();
        target.set("usedTimes", count + 1);
    }
    return self;
}
JSValue remove(const JSValue& self, const Args& args, const Args&) {
    const auto material = argument(args, 0);
    const auto shaders = call(self.get("materialCache"), "get", {material});
    auto iterator = getJSIterator(shaders);
    while (const auto stage = stepJSIterator(iterator)) {
        try {
            auto target = *stage;
            const auto count = target.get("usedTimes").toNumber();
            target.set("usedTimes", count - 1);
            if (target.get("usedTimes").strictEquals(0)) {
                const auto cache = self.get("shaderCache");
                const auto erase = cache.get("delete");
                const auto code = target.get("code");
                erase.call(cache, {code});
            }
        } catch (...) {
            closeJSIterator(iterator, std::current_exception());
        }
    }
    call(self.get("materialCache"), "delete", {material});
    return self;
}
JSValue vertex(const JSValue& self, const Args& args, const Args&) {
    const auto method = self.get("_getShaderStage");
    const auto code = argument(args, 0).get("vertexShader");
    return method.call(self, {code});
}
JSValue fragment(const JSValue& self, const Args& args, const Args&) {
    const auto method = self.get("_getShaderStage");
    const auto code = argument(args, 0).get("fragmentShader");
    return method.call(self, {code});
}
JSValue dispose(const JSValue& self, const Args&, const Args&) {
    call(self.get("shaderCache"), "clear");
    call(self.get("materialCache"), "clear");
    return {};
}
JSValue materialShaders(const JSValue& self, const Args& args, const Args&) {
    const auto material = argument(args, 0), cache = self.get("materialCache");
    auto set = call(cache, "get", {material});
    if (set.isUndefined()) {
        set = createNativeSet();
        call(cache, "set", {material, set});
    }
    return set;
}
JSValue shaderStage(const JSValue& self, const Args& args, const Args& captures) {
    const auto code = argument(args, 0), cache = self.get("shaderCache");
    auto stage = call(cache, "get", {code});
    if (stage.isUndefined()) {
        stage = captures[0].construct({code});
        call(cache, "set", {code, stage});
    }
    return stage;
}
void check(const JSValue& value) { static_cast<void>(JSWeakValue(value)); }
} // namespace

JSValue nativeWebGLShaderCacheConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isCallable()) return *value;
    auto state = JSValue::object(nullptr);
    JSPropertyDescriptor id;
    id.value = 0;
    id.writable = id.enumerable = id.configurable = true;
    state.defineProperty("id", id);
    const auto stage = JSValue::constructor(initializeStage, {state}, "WebGLShaderStage", 1);
    // The ESM module keeps its unexported class binding after users replace or
    // delete _getShaderStage. Retain that module edge independently of methods.
    auto constructor = JSValue::constructor(initializeCache, {stage}, "WebGLShaderCache", 0);
    auto prototype = constructor.get("prototype");
    method(prototype, "update", update, 3);
    method(prototype, "remove", remove, 1);
    method(prototype, "getVertexShaderStage", vertex, 1);
    method(prototype, "getFragmentShaderStage", fragment, 1);
    method(prototype, "dispose", dispose, 0);
    method(prototype, "_getShaderCacheForMaterial", materialShaders, 1);
    method(prototype, "_getShaderStage", shaderStage, 1, {stage});
    constructor.retainForHeapLifetime();
    weak = JSWeakValue(constructor);
    return constructor;
}
WebGLShaderCache::WebGLShaderCache() : object_(nativeWebGLShaderCacheConstructor().construct()) {}
JSValue WebGLShaderCache::update(const JSValue& material, const JSValue& vertex,
                                const JSValue& fragment) {
    check(object_); check(material); check(vertex); check(fragment);
    return call(object_, "update", {material, vertex, fragment});
}
JSValue WebGLShaderCache::remove(const JSValue& material) {
    check(object_); check(material); return call(object_, "remove", {material});
}
JSValue WebGLShaderCache::getVertexShaderStage(const JSValue& material) {
    check(object_); check(material); return call(object_, "getVertexShaderStage", {material});
}
JSValue WebGLShaderCache::getFragmentShaderStage(const JSValue& material) {
    check(object_); check(material); return call(object_, "getFragmentShaderStage", {material});
}
JSValue WebGLShaderCache::dispose() { check(object_); return call(object_, "dispose"); }
JSValue WebGLShaderCache::_getShaderCacheForMaterial(const JSValue& material) {
    check(object_); check(material); return call(object_, "_getShaderCacheForMaterial", {material});
}
JSValue WebGLShaderCache::_getShaderStage(const JSValue& code) {
    check(object_); check(code); return call(object_, "_getShaderStage", {code});
}
