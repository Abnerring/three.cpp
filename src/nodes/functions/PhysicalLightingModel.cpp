#include "nodes/functions/PhysicalLightingModel.h"

#include "core/JSWeakMap.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"

#include <stdexcept>
#include <string>

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
JSWeakValue& baseKeeper() { static thread_local JSWeakValue weak; return weak; }

[[noreturn]] JSValue notPorted(const char* name) {
    throw std::runtime_error(std::string("PhysicalLightingModel: ") + name + " is not ported yet");
}

JSWeakValue& helperCache() { static thread_local JSWeakValue weak; return weak; }

JSValue floatNode(double value) { return nativeTSLBaseBinding("float").call({}, {JSValue(value)}); }
JSValue vec3Node(const JSValue& x, const JSValue& y, const JSValue& z) {
    return nativeTSLBaseBinding("vec3").call({}, {x, y, z});
}

// The Fn convention: the inputs arrive as one iterable whose single entry is the destructured object or array.
JSValue inputObject(const Args& args) {
    auto iterator = getJSIterator(arg(args, 0));
    JSValue inputs;
    if (!iterator.done) {
        const auto value = stepJSIterator(iterator);
        if (value) inputs = *value;
    }
    if (!iterator.done) closeJSIterator(iterator);
    return inputs;
}

JSValue makeFn(JSValue::NativeFunction body) {
    auto callback = JSValue::function(body, {}, "", 0);
    return nativeTSLCoreBinding("Fn").call({}, {callback});
}

// applyIorToRoughness = Fn( ( [ roughness, ior ] ) => roughness.mul( clamp( ior.mul( 2 ).sub( 2 ), 0, 1 ) ) )
JSValue applyIorToRoughnessBody(const JSValue&, const Args& args, const Args&) {
    const auto roughness = arg(args, 0);
    const auto ior = arg(args, 1);
    auto scaled = invoke(ior, "mul")({JSValue(2.0)});
    scaled = invoke(scaled, "sub")({JSValue(2.0)});
    const auto clamped = nativeMathNodeBinding("clamp").call({}, {scaled, JSValue(0.0), JSValue(1.0)});
    return invoke(roughness, "mul")({clamped});
}

// volumeAttenuation = Fn( ( [ transmissionDistance, attenuationColor, attenuationDistance ] ) => { If(...) } )
JSValue volumeAttenuationBody(const JSValue&, const Args& args, const Args&) {
    const auto transmissionDistance = arg(args, 0);
    const auto attenuationColor = arg(args, 1);
    const auto attenuationDistance = arg(args, 2);

    auto result = invoke(vec3Node(JSValue(1.0), JSValue(1.0), JSValue(1.0)), "toVar")();
    const auto condition =
        invoke(attenuationDistance, "notEqual")({JSValue(0.0)});
    auto body = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            const auto target = captures.at(0);
            const auto transmissionDistance = captures.at(1);
            const auto attenuationColor = captures.at(2);
            const auto attenuationDistance = captures.at(3);
            auto coefficient = nativeMathNodeBinding("log").call({}, {attenuationColor});
            coefficient = invoke(coefficient, "negate")();
            coefficient = invoke(coefficient, "div")({attenuationDistance});
            auto exponent = invoke(coefficient, "negate")();
            exponent = invoke(exponent, "mul")({transmissionDistance});
            const auto transmittance = nativeMathNodeBinding("exp").call({}, {exponent});
            invoke(target, "assign")({transmittance});
            return JSValue{};
        },
        {result, transmissionDistance, attenuationColor, attenuationDistance});
    nativeTSLBaseBinding("If").call({}, {condition}).call({}, {body});
    return result;
}

// getVolumeTransmissionRay = Fn( ( [ n, v, thickness, ior, modelMatrix ] ) => ... )
JSValue getVolumeTransmissionRayBody(const JSValue&, const Args& args, const Args&) {
    const auto n = arg(args, 0);
    const auto v = arg(args, 1);
    const auto thickness = arg(args, 2);
    const auto ior = arg(args, 3);
    const auto modelMatrix = arg(args, 4);

    const auto negatedV = invoke(v, "negate")();
    const auto normalizedN = nativeMathNodeBinding("normalize").call({}, {n});
    const auto eta = invoke(floatNode(1.0), "div")({ior});
    const auto refracted = nativeMathNodeBinding("refract").call({}, {negatedV, normalizedN, eta});
    const auto refractionVector = vec3Node(refracted.get("x"), refracted.get("y"), refracted.get("z"));

    auto modelScaleComponents = JSValue::array(3);
    for (int row = 0; row < 3; ++row) {
        const auto column = invoke(modelMatrix, "element")({JSValue(static_cast<double>(row))});
        modelScaleComponents.set(static_cast<std::size_t>(row),
                                 nativeMathNodeBinding("length").call({}, {column.get("xyz")}));
    }
    const auto modelScale = nativeTSLBaseBinding("vec3")
                                .call({}, {modelScaleComponents.get(JSValue(0.0)),
                                           modelScaleComponents.get(JSValue(1.0)),
                                           modelScaleComponents.get(JSValue(2.0))});
    const auto scaledThickness = invoke(thickness, "mul")({modelScale});
    return invoke(nativeMathNodeBinding("normalize").call({}, {refractionVector}), "mul")({scaledThickness});
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

// Port only hook: builds one module level helper graph by name.
JSValue nativePhysicalLightingModelHelper(const std::string& name) {
    if (name == "applyIorToRoughness") return makeFn(applyIorToRoughnessBody);
    if (name == "volumeAttenuation") return makeFn(volumeAttenuationBody);
    if (name == "getVolumeTransmissionRay") return makeFn(getVolumeTransmissionRayBody);
    return JSValue{};
}