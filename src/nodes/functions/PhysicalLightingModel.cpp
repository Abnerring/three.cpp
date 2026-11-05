#include "nodes/functions/PhysicalLightingModel.h"

#include "core/JSWeakMap.h"
#include "nodes/lighting/LightingNodeRuntime.h"

#include <stdexcept>
#include <string>

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
JSWeakValue& baseKeeper() { static thread_local JSWeakValue weak; return weak; }

[[noreturn]] JSValue notPorted(const char* name) {
    throw std::runtime_error(std::string("PhysicalLightingModel: ") + name + " is not ported yet");
}

// The base LightingModel constructor: the pinned class only declares the hooks, so the base answers
// undefined and the concrete model provides every hook it uses.
JSValue baseInitialize(const JSValue& target, const Args&, const Args&) { return target; }
JSValue baseMethod(const JSValue&, const Args&, const Args&) { return JSValue{}; }

JSValue makeLightingModelBase() {
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(baseInitialize, {holder}, "LightingModel", 0);
    holder.set("constructor", ctor);
    auto prototype = ctor.get("prototype");
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
             {"start", 1}, {"finish", 0}, {"direct", 0}, {"directRectArea", 0},
             {"indirect", 0}, {"ambientOcclusion", 0}}) {
        auto fn = JSValue::function(baseMethod, {name}, name, arity);
        JSPropertyDescriptor descriptor;
        descriptor.value = fn;
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }
    baseKeeper() = JSWeakValue(ctor);
    return ctor;
}

JSValue stubStart(const JSValue&, const Args&, const Args&) { return notPorted("start"); }
JSValue stubComputeMultiscattering(const JSValue&, const Args&, const Args&) {
    return notPorted("computeMultiscattering");
}
JSValue stubDirect(const JSValue&, const Args&, const Args&) { return notPorted("direct"); }
JSValue stubDirectRectArea(const JSValue&, const Args&, const Args&) {
    return notPorted("directRectArea");
}
JSValue stubIndirect(const JSValue&, const Args&, const Args&) { return notPorted("indirect"); }
JSValue stubIndirectDiffuse(const JSValue&, const Args&, const Args&) {
    return notPorted("indirectDiffuse");
}
JSValue stubIndirectSpecular(const JSValue&, const Args&, const Args&) {
    return notPorted("indirectSpecular");
}
JSValue stubAmbientOcclusion(const JSValue&, const Args&, const Args&) {
    return notPorted("ambientOcclusion");
}
JSValue stubFinish(const JSValue&, const Args&, const Args&) { return notPorted("finish"); }
}  // namespace

JSValue nativePhysicalLightingModelModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "PhysicalLightingModel module");

    const auto base = makeLightingModelBase();
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = captures[0].get("constructor").getPrototype().construct({}, target);
            const auto flag = [&args](std::size_t index, bool fallback) {
                const auto value = arg(args, index, JSValue{});
                return value.isUndefined() ? fallback : value.toBoolean();
            };
            self.set("clearcoat", JSValue(flag(0, false)));
            self.set("sheen", JSValue(flag(1, false)));
            self.set("iridescence", JSValue(flag(2, false)));
            self.set("anisotropy", JSValue(flag(3, false)));
            self.set("transmission", JSValue(flag(4, false)));
            self.set("dispersion", JSValue(flag(5, false)));
            self.set("retroreflection", JSValue(flag(6, false)));
            self.set("diffuseRoughness", JSValue(flag(7, false)));
            for (const char* field : {"clearcoatRadiance", "clearcoatSpecularDirect",
                                      "clearcoatSpecularIndirect", "sheenSpecularDirect",
                                      "sheenSpecularIndirect", "iridescenceFresnel",
                                      "iridescenceF0Dielectric", "iridescenceF0Metallic", "dfg",
                                      "eonDirectionalAlbedo", "multiScatteringCompensation",
                                      "singleScatteringDielectric", "multiScatteringDielectric"}) {
                self.set(field, JSValue(nullptr));
            }
            return self;
        },
        {holder}, "PhysicalLightingModel", 9);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));

    for (const auto& [name, arity, callback] : std::initializer_list<
             std::tuple<const char*, unsigned, JSValue::NativeFunction>>{
             {"start", 1, stubStart},
             {"computeMultiscattering", 5, stubComputeMultiscattering},
             {"direct", 1, stubDirect},
             {"directRectArea", 1, stubDirectRectArea},
             {"indirect", 1, stubIndirect},
             {"indirectDiffuse", 1, stubIndirectDiffuse},
             {"indirectSpecular", 1, stubIndirectSpecular},
             {"ambientOcclusion", 1, stubAmbientOcclusion},
             {"finish", 1, stubFinish}}) {
        auto fn = JSValue::function(callback, {}, name, arity);
        JSPropertyDescriptor descriptor;
        descriptor.value = fn;
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }

    JSValue bindings = JSValue::object(nullptr);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativePhysicalLightingModelBinding(const std::string& name) {
    const auto keeper = nativePhysicalLightingModelModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativePhysicalLightingModelConstructor() {
    return nativePhysicalLightingModelBinding("default");
}