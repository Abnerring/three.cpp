#include "nodes/lighting/ShadowNode.h"

#include "core/JSWeakMap.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/lighting/ShadowBaseNode.h"
#include "nodes/lighting/ShadowFilterNode.h"

#include <stdexcept>
#include <string>

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// const _shadowFilterLib = [ BasicShadowFilter, PCFShadowFilter, null, VSMShadowFilter ]
JSValue shadowFilterLibrary() {
    auto library = JSValue::array(4);
    library.set(JSValue(0.0), nativeShadowFilterNodeBinding("BasicShadowFilter"));
    library.set(JSValue(1.0), nativeShadowFilterNodeBinding("PCFShadowFilter"));
    library.set(JSValue(2.0), JSValue(nullptr));
    library.set(JSValue(3.0), nativeShadowFilterNodeBinding("VSMShadowFilter"));
    return library;
}

// getShadowFilterFn( type ) { return _shadowFilterLib[ type ]; }
JSValue getShadowFilterFn(const JSValue&, const Args& args, const Args&) {
    return shadowFilterLibrary().get(arg(args, 0).toNumber());
}

[[noreturn]] JSValue notPorted(const char* name) {
    throw std::runtime_error(std::string("ShadowNode: ") + name + " is not ported yet");
}

// dispose() { this._reset(); super.dispose(); }
JSValue disposeMethod(const JSValue& rawSelf, const Args&, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto reset = self.get("_reset");
    if (reset.toBoolean()) reset.call(self, {});
    const auto basePrototype = nativeShadowBaseNodeConstructor().get("prototype");
    const auto baseDispose = basePrototype.get("dispose");
    if (baseDispose.toBoolean()) baseDispose.call(self, {});
    return {};
}

void disposeCache(JSValue& self, const char* name) {
    JSValue slot = self.get(name);
    if (!slot.toBoolean()) return;
    const auto dispose = slot.get("dispose");
    if (dispose.toBoolean()) dispose.call(slot, {});
    self.set(name, JSValue(nullptr));
}

// _reset() { ...dispose the cached shadow maps and VSM materials... }
JSValue resetMethod(const JSValue& rawSelf, const Args&, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    self.set("_currentShadowType", JSValue(nullptr));
    if (const auto disposeShadowMaterial = self.get("disposeShadowMaterial");
        disposeShadowMaterial.toBoolean()) {
        disposeShadowMaterial.call(self, {});
    }
    disposeCache(self, "shadowMap");
    disposeCache(self, "vsmShadowMapVertical");
    disposeCache(self, "vsmShadowMapHorizontal");
    disposeCache(self, "vsmMaterialVertical");
    disposeCache(self, "vsmMaterialHorizontal");
    return {};
}

JSValue setupShadowFilter(const JSValue&, const Args&, const Args&) { return notPorted("setupShadowFilter"); }
JSValue setupShadowCoord(const JSValue&, const Args&, const Args&) { return notPorted("setupShadowCoord"); }
JSValue setupRenderTarget(const JSValue&, const Args&, const Args&) { return notPorted("setupRenderTarget"); }
JSValue setupShadow(const JSValue&, const Args&, const Args&) { return notPorted("setupShadow"); }
JSValue setup(const JSValue&, const Args&, const Args&) { return notPorted("setup"); }
JSValue renderShadow(const JSValue&, const Args&, const Args&) { return notPorted("renderShadow"); }
JSValue updateShadow(const JSValue&, const Args&, const Args&) { return notPorted("updateShadow"); }
JSValue vsmPass(const JSValue&, const Args&, const Args&) { return notPorted("vsmPass"); }
JSValue updateBefore(const JSValue&, const Args&, const Args&) { return notPorted("updateBefore"); }
}  // namespace

JSValue nativeShadowNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ShadowNode module");
    nativeShadowBaseNodeModule();
    nativeShadowFilterNodeModule();

    const auto base = nativeShadowBaseNodeConstructor();
    JSValue bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "ShadowNode", 2, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            const auto light = arg(args, 0, nullptr);
            auto self = superConstruct(target, captures, {light});
            auto shadow = arg(args, 1, JSValue{});
            if (!shadow.toBoolean()) shadow = light.isObject() ? light.get("shadow") : JSValue{};
            self.set("shadow", shadow);
            self.set("shadowMap", JSValue(nullptr));
            self.set("vsmShadowMapVertical", JSValue(nullptr));
            self.set("vsmShadowMapHorizontal", JSValue(nullptr));
            self.set("vsmMaterialVertical", JSValue(nullptr));
            self.set("vsmMaterialHorizontal", JSValue(nullptr));
            self.set("_node", JSValue(nullptr));
            self.set("_currentShadowType", JSValue(nullptr));
            self.set("_cameraFrameId", createNativeWeakMap());
            self.set("isShadowNode", true);
            self.set("depthLayer", JSValue(0));
            return self;
        },
        {}, "", "");

    const auto prototype = ctor.get("prototype");
    method(prototype, "getShadowFilterFn", 1, getShadowFilterFn, {}, "");
    method(prototype, "dispose", 0, disposeMethod, {}, "");
    method(prototype, "_reset", 0, resetMethod, {}, "");
    method(prototype, "setupShadowFilter", 2, setupShadowFilter, {}, "");
    method(prototype, "setupShadowCoord", 2, setupShadowCoord, {}, "");
    method(prototype, "setupRenderTarget", 2, setupRenderTarget, {}, "");
    method(prototype, "setupShadow", 1, setupShadow, {}, "");
    method(prototype, "setup", 1, setup, {}, "");
    method(prototype, "renderShadow", 1, renderShadow, {}, "");
    method(prototype, "updateShadow", 1, updateShadow, {}, "");
    method(prototype, "vsmPass", 1, vsmPass, {}, "");
    method(prototype, "updateBefore", 1, updateBefore, {}, "");
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeShadowNodeBinding(const std::string& name) {
    const auto keeper = nativeShadowNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeShadowNodeConstructor() { return nativeShadowNodeBinding("default"); }