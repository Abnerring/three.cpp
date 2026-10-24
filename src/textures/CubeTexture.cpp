#include "textures/CubeTexture.h"
#include "core/JSModuleObject.h"
CubeTexture::CubeTexture(Image images, double mapping, double wrapS, double wrapT, double magFilter, double minFilter,
                         double format, double type, double anisotropy, std::string colorSpace)
    : Texture(std::move(images), mapping, wrapS, wrapT, magFilter, minFilter, format, type, anisotropy, std::move(colorSpace)) {
    isCubeTexture = true; flipY = false;
}
std::shared_ptr<Texture> CubeTexture::clone() const {
    auto result = std::make_shared<CubeTexture>(); result->copy(*this); return result;
}

JSValue nativeCubeTextureConstructor() {
    using Args = std::vector<JSValue>;
    static thread_local JSWeakValue weak; static thread_local bool initializing = false;
    if (const auto ctor = weak.lock(); ctor && ctor->isConstructible()) return *ctor;
    JSModuleInitializationScope scope(initializing, "CubeTexture"); auto state = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        const auto get = [&](std::size_t i) { return i < args.size() ? args[i] : JSValue{}; };
        auto images = get(0); if (images.isUndefined()) images = JSValue::array();
        auto mapping = get(1); if (mapping.isUndefined()) mapping = CubeReflectionMapping;
        Args values{images, mapping}; for (unsigned i = 2; i < 10; ++i) values.push_back(get(i));
        auto self = captures[0].get("constructor").getPrototype().construct(values, newTarget);
        self.set("isCubeTexture", true); self.set("flipY", false); return self;
    }, {state}, "CubeTexture", 0);
    state.set("constructor", ctor); const auto base = nativeTextureConstructor(); ctor.setPrototype(base);
    auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor d; d.configurable = true;
    d.getter = JSValue::function([](const JSValue& self, const Args&, const Args&) -> JSValue { return self.get("image"); }, {}, "get images", 0);
    d.setter = JSValue::function([](const JSValue& receiver, const Args& args, const Args&) -> JSValue { auto self = receiver; self.set("image", args.empty() ? JSValue{} : args[0]); return {}; }, {}, "set images", 1);
    prototype.defineProperty("images", d); ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
