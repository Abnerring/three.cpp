#include "core/RenderTarget.h"
#include "core/JSModuleObject.h"
#include "core/JSOperators.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue defaulted(const Args& args, std::size_t i, JSValue fallback) {
    const auto value = arg(args, i); return value.isUndefined() ? fallback : value;
}
JSValue invoke(const JSValue& self, const char* name, const Args& args = {}) {
    const auto fn = self.get(name); return fn.call(self, args);
}
void literalField(JSValue value, const char* key, const JSValue& data) {
    JSPropertyDescriptor d; d.value = data; d.writable = d.enumerable = d.configurable = true; value.defineProperty(key, d);
}
JSValue attachment(const JSValue& self, std::size_t index) {
    return self.get("textures").get(JSValue(index));
}
void setAttachmentField(const JSValue& self, std::size_t index, const char* key, const JSValue& value) {
    auto texture = attachment(self, index); texture.set(key, value);
}
const std::initializer_list<const char*> flags = {"depthBuffer", "stencilBuffer", "resolveColorBuffer",
    "resolveDepthBuffer", "resolveStencilBuffer", "storeMultisampledColorBuffer",
    "storeMultisampledDepthBuffer", "storeMultisampledStencilBuffer"};
JSValue targetMethod(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string();
    if (name == "get texture") return self.get("textures").get("0");
    if (name == "set texture") { auto textures = self.get("textures"); textures.set("0", arg(args, 0)); return {}; }
    if (name == "get depthTexture") return self.get("_depthTexture");
    if (name == "set depthTexture") {
        const auto current = arg(args, 0);
        if (!self.get("_depthTexture").isNull() && self.get("_depthTexture").get("renderTarget").strictEquals(self)) {
            auto previous = self.get("_depthTexture"); previous.set("renderTarget", nullptr);
        }
        if (!current.isNull() && current.get("renderTarget").isNull()) { auto texture = current; texture.set("renderTarget", self); }
        self.set("_depthTexture", current); return {};
    }
    if (name == "_setTextureOptions") {
        const auto options = defaulted(args, 0, JSValue::object());
        auto values = JSValue::object(); literalField(values, "minFilter", LinearFilter); literalField(values, "generateMipmaps", false);
        literalField(values, "flipY", false); literalField(values, "internalFormat", nullptr);
        for (const auto* key : {"mapping", "wrapS", "wrapT", "wrapR", "magFilter", "minFilter", "format", "type",
                 "anisotropy", "colorSpace", "flipY", "generateMipmaps", "mipmapsAutoUpdate", "internalFormat"})
            if (!options.get(key).isUndefined()) values.set(key, options.get(key));
        for (std::size_t i = 0; JSOperators::less(i, self.get("textures").get("length")); ++i) {
            const auto texture = attachment(self, i); invoke(texture, "setValues", {values});
        }
        return {};
    }
    if (name == "setSize") {
        const auto width = arg(args, 0), height = arg(args, 1), depth = defaulted(args, 2, 1);
        if (!self.get("width").strictEquals(width) || !self.get("height").strictEquals(height) || !self.get("depth").strictEquals(depth)) {
            self.set("width", width); self.set("height", height); self.set("depth", depth);
            const auto length = self.get("textures").get("length");
            for (std::size_t i = 0; JSOperators::less(i, length); ++i) {
                for (const auto& [key, value] : std::initializer_list<std::pair<const char*, JSValue>>{{"width", width}, {"height", height}, {"depth", depth}}) {
                    auto image = attachment(self, i).get("image"); image.set(key, value);
                }
                if (!attachment(self, i).get("isData3DTexture").strictEquals(true)) {
                    // Assignment reference precedes evaluation of the depth getter.
                    auto destination = attachment(self, i);
                    const auto imageDepth = attachment(self, i).get("image").get("depth");
                    destination.set("isArrayTexture", JSOperators::greater(imageDepth, 1));
                }
            }
            invoke(self, "dispose");
        }
        invoke(self.get("viewport"), "set", {0, 0, width, height});
        invoke(self.get("scissor"), "set", {0, 0, width, height}); return {};
    }
    if (name == "clone") { auto result = self.get("constructor").construct(); return invoke(result, "copy", {self}); }
    if (name == "dispose") { auto event = JSValue::object(); literalField(event, "type", "dispose"); invoke(self, "dispatchEvent", {event}); return {}; }
    if (name == "copy") {
        const auto source = arg(args, 0);
        for (const auto* key : {"width", "height", "depth"}) self.set(key, source.get(key));
        auto rect = self.get("scissor"); auto fn = rect.get("copy"); fn.call(rect, {source.get("scissor")});
        self.set("scissorTest", source.get("scissorTest"));
        rect = self.get("viewport"); fn = rect.get("copy"); fn.call(rect, {source.get("viewport")});
        auto textures = self.get("textures"); textures.set("length", 0);
        const auto length = source.get("textures").get("length");
        for (std::size_t i = 0; JSOperators::less(i, length); ++i) {
            auto destination = self.get("textures");
            const auto original = attachment(source, i); destination.set(JSValue(i), invoke(original, "clone"));
            setAttachmentField(self, i, "isRenderTargetTexture", true); setAttachmentField(self, i, "renderTarget", self);
            auto image = JSValue::object(); const auto object = JSValue::intrinsicObjectConstructor();
            const auto assign = object.get("assign"); image = assign.call(object, {image, attachment(source, i).get("image")});
            auto texture = attachment(self, i); texture.set("source", nativeTextureSourceConstructor().construct({image}));
        }
        for (const auto* key : flags) self.set(key, source.get(key));
        if (!source.get("depthTexture").isNull()) {
            if (source.get("depthTexture").get("renderTarget").strictEquals(source)) {
                auto depthTexture = invoke(source.get("depthTexture"), "clone"); depthTexture.set("renderTarget", nullptr);
                self.set("depthTexture", depthTexture);
            } else self.set("depthTexture", source.get("depthTexture"));
        } else self.set("depthTexture", nullptr);
        for (const auto* key : {"samples", "multiview", "useArrayDepthTexture"}) self.set(key, source.get(key));
        return self;
    }
    throw std::logic_error("Unknown RenderTarget method");
}
}

JSValue nativeRenderTargetConstructor() {
    static thread_local JSWeakValue weak; static thread_local bool initializing = false;
    if (const auto ctor = weak.lock(); ctor && ctor->isConstructible()) return *ctor;
    JSModuleInitializationScope scope(initializing, "RenderTarget");
    const auto base = EventDispatcher::runtimePrototype().get("constructor");
    auto state = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        const auto width = defaulted(args, 0, 1), height = defaulted(args, 1, 1);
        auto self = captures[0].get("constructor").getPrototype().construct({}, newTarget);
        auto defaults = JSValue::object();
        for (const auto& [key, value] : std::initializer_list<std::pair<const char*, JSValue>>{
                 {"generateMipmaps", false}, {"internalFormat", nullptr}, {"minFilter", LinearFilter},
                 {"depthBuffer", true}, {"stencilBuffer", false}, {"resolveColorBuffer", true},
                 {"resolveDepthBuffer", true}, {"resolveStencilBuffer", true}, {"storeMultisampledColorBuffer", true},
                 {"storeMultisampledDepthBuffer", true}, {"storeMultisampledStencilBuffer", true},
                 {"depthTexture", nullptr}, {"samples", 0}, {"count", 1}, {"depth", 1}, {"multiview", false}, {"useArrayDepthTexture", false}})
            literalField(defaults, key, value);
        const auto object = JSValue::intrinsicObjectConstructor();
        const auto options = object.get("assign").call(object, {defaults, defaulted(args, 2, JSValue::object())});
        self.set("isRenderTarget", true); self.set("width", width); self.set("height", height); self.set("depth", options.get("depth"));
        self.set("scissor", nativeVector4Constructor().construct({0, 0, width, height})); self.set("scissorTest", false);
        self.set("viewport", nativeVector4Constructor().construct({0, 0, width, height})); self.set("textures", JSValue::array());
        auto image = JSValue::object(); literalField(image, "width", width); literalField(image, "height", height); literalField(image, "depth", options.get("depth"));
        const auto texture = nativeTextureConstructor().construct({image}), count = options.get("count");
        for (std::size_t i = 0; JSOperators::less(i, count); ++i) {
            auto destination = self.get("textures"); destination.set(JSValue(i), invoke(texture, "clone"));
            setAttachmentField(self, i, "isRenderTargetTexture", true); setAttachmentField(self, i, "renderTarget", self);
        }
        invoke(self, "_setTextureOptions", {options});
        for (const auto* key : flags) self.set(key, options.get(key));
        self.set("_depthTexture", nullptr); self.set("depthTexture", options.get("depthTexture"));
        for (const auto* key : {"samples", "multiview", "useArrayDepthTexture"}) self.set(key, options.get(key));
        return self;
    }, {state}, "RenderTarget", 0);
    state.set("constructor", ctor);
    ctor.setPrototype(base); auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    for (const auto* name : {"_setTextureOptions", "texture", "depthTexture", "setSize", "clone", "copy", "dispose"}) {
        JSPropertyDescriptor d; d.configurable = true;
        if (std::string(name) == "texture" || std::string(name) == "depthTexture") {
            d.getter = JSValue::function(targetMethod, {std::string("get ") + name}, std::string("get ") + name, 0);
            d.setter = JSValue::function(targetMethod, {std::string("set ") + name}, std::string("set ") + name, 1);
        } else {
            const auto arity = std::string(name) == "setSize" ? 2U : std::string(name) == "copy" ? 1U : 0U;
            d.value = JSValue::function(targetMethod, {name}, name, arity); d.writable = true;
        }
        prototype.defineProperty(name, d);
    }
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
