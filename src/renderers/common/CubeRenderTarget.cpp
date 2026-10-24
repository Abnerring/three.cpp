#include "renderers/common/CubeRenderTarget.h"
#include "core/RenderTarget.h"
#include "core/JSModuleObject.h"
#include "textures/CubeTexture.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue defaulted(const Args& args, std::size_t i, const JSValue& fallback) { const auto value = arg(args, i); return value.isUndefined() ? fallback : value; }
JSValue invoke(const JSValue& self, const char* key, const Args& args = {}) { const auto fn = self.get(key); return fn.call(self, args); }
JSValue cubeMethod(const JSValue& self, const Args& args, const Args& captures) {
    if (captures[0].string() == "fromEquirectangularTexture")
        throw JSTypeError("CubeRenderTarget.fromEquirectangularTexture requires the pending NodeMaterial and cube render path");
    const auto renderer = arg(args, 0), color = defaulted(args, 1, true), depth = defaulted(args, 2, true), stencil = defaulted(args, 3, true);
    const auto previous = invoke(renderer, "getRenderTarget");
    for (unsigned i = 0; i < 6; ++i) { invoke(renderer, "setRenderTarget", {self, i}); invoke(renderer, "clear", {color, depth, stencil}); }
    invoke(renderer, "setRenderTarget", {previous}); return {};
}
}
JSValue nativeCubeRenderTargetConstructor() {
    static thread_local JSWeakValue weak; static thread_local bool initializing = false;
    if (const auto ctor = weak.lock(); ctor && ctor->isConstructible()) return *ctor;
    JSModuleInitializationScope scope(initializing, "CubeRenderTarget"); auto state = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        const auto size = defaulted(args, 0, 1), options = defaulted(args, 1, JSValue::object());
        auto self = captures[0].get("constructor").getPrototype().construct({size, size, options}, newTarget);
        self.set("isCubeRenderTarget", true); auto image = JSValue::object();
        for (const auto& [key, value] : std::initializer_list<std::pair<const char*, JSValue>>{{"width", size}, {"height", size}, {"depth", 1}}) { JSPropertyDescriptor d; d.value = value; d.writable = d.enumerable = d.configurable = true; image.defineProperty(key, d); }
        auto images = JSValue::array(); for (unsigned i = 0; i < 6; ++i) { JSPropertyDescriptor d; d.value = image; d.writable = d.enumerable = d.configurable = true; images.defineProperty(JSValue(i), d); }
        self.set("texture", nativeCubeTextureConstructor().construct({images})); invoke(self, "_setTextureOptions", {options});
        auto texture = self.get("texture"); texture.set("isRenderTargetTexture", true); return self;
    }, {state}, "CubeRenderTarget", 0);
    state.set("constructor", ctor); const auto base = nativeRenderTargetConstructor(); ctor.setPrototype(base);
    auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"fromEquirectangularTexture", 2}, {"clear", 1}}) {
        JSPropertyDescriptor d; d.value = JSValue::function(cubeMethod, {name}, name, arity); d.writable = d.configurable = true; prototype.defineProperty(name, d);
    }
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
