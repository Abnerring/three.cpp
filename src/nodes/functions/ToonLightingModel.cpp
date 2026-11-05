#include "nodes/functions/ToonLightingModel.h"

#include "nodes/accessors/MaterialReferenceNode.h"
#include "nodes/accessors/Normal.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/functions/BSDF/BRDF_Lambert.h"
#include "nodes/functions/NodeFunctionSources.inc"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"

#include <initializer_list>
#include <tuple>

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}
JSValue& baseKeeper() {
    static thread_local JSValue value;
    return value;
}
JSWeakValue& gradientCache() {
    static thread_local JSWeakValue value;
    return value;
}

JSValue baseInitialize(const JSValue& newTarget, const Args&, const Args&) {
    return JSValue::object(newTarget.get("prototype"));
}

// start sets the lights up and calls indirect; the other five hooks are abstract until overridden.
JSValue baseMethod(const JSValue& receiver, const Args& args, const Args& captures) {
    if (captures[0].toString() == "start") {
        const auto builder = arg(args, 0);
        const auto lights = builder.get("lightsNode");
        invoke(lights, "setupLights")({builder, invoke(lights, "getLightNodes")({builder})});
        invoke(receiver, "indirect")({builder});
    }
    return {};
}

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
    baseKeeper() = ctor;
    return ctor;
}

JSValue vec3Node(const JSValue& value) {
    return nativeTSLBaseBinding("vec3").call({}, {value});
}

// getGradientIrradiance = Fn( ( { normal, lightDirection, builder } ) => { ... } )
JSValue getGradientIrradianceBody(const JSValue&, const Args& args, const Args&) {
    auto iterator = getJSIterator(arg(args, 0));
    JSValue inputs;
    if (!iterator.done) {
        const auto value = stepJSIterator(iterator);
        if (value) inputs = *value;
    }
    if (!iterator.done) closeJSIterator(iterator);

    const auto normal = inputs.get("normal");
    const auto lightDirection = inputs.get("lightDirection");
    const auto builder = inputs.get("builder");

    const auto dotNL = invoke(normal, "dot")({lightDirection});
    // vec2( dotNL.mul( 0.5 ).add( 0.5 ), 0.0 )
    const auto coord = nativeTSLBaseBinding("vec2").call(
        {}, {invoke(invoke(dotNL, "mul")({JSValue(0.5)}), "add")({JSValue(0.5)}), JSValue(0.0)});

    if (builder.get("material").get("gradientMap").toBoolean()) {
        // materialReference( 'gradientMap', 'texture' ).context( { getUV: () => coord } )
        const auto reference = nativeMaterialReferenceNodeBinding("materialReference").call(
            {}, {JSValue("gradientMap"), JSValue("texture")});
        auto state = JSValue::object(nullptr);
        state.set("coord", coord);
        auto contextConfig = JSValue::object(nullptr);
        contextConfig.set("getUV", JSValue::function(
            +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
                return captures[0];
            },
            {state.get("coord")}, "getUV", 0));
        const auto sampled = invoke(reference, "context")({contextConfig});
        return vec3Node(sampled.get("r"));
    }

    // const fw = coord.fwidth().mul( 0.5 )
    const auto fw = invoke(invoke(coord, "fwidth")(), "mul")({JSValue(0.5)});
    const auto low = invoke(nativeTSLBaseBinding("float").call({}, {JSValue(0.7)}), "sub")({fw.get("x")});
    const auto high = invoke(nativeTSLBaseBinding("float").call({}, {JSValue(0.7)}), "add")({fw.get("x")});
    const auto stepped =
        nativeMathNodeBinding("smoothstep").call({}, {low, high, coord.get("x")});
    return nativeMathNodeBinding("mix").call({}, {vec3Node(JSValue(0.7)), vec3Node(JSValue(1.0)), stepped});
}

JSValue gradientIrradiance() {
    if (const auto cached = gradientCache().lock(); cached && cached->isCallable()) return *cached;
    auto callback = JSValue::function(getGradientIrradianceBody, {}, "", 0);
    const auto fn = nativeTSLCoreBinding("Fn").call({}, {callback});
    gradientCache() = JSWeakValue(fn);
    return fn;
}

JSValue brdfLambert(const JSValue& diffuse) {
    auto inputs = JSValue::object(nullptr);
    inputs.set("diffuseColor", diffuse);
    return nativeBRDFLambert().call({}, {inputs});
}

// direct( { lightDirection, lightColor, reflectedLight }, builder ) { ... }
JSValue direct(const JSValue&, const Args& args, const Args&) {
    const auto inputs = arg(args, 0);
    const auto builder = arg(args, 1);

    auto gradientInputs = JSValue::object(nullptr);
    gradientInputs.set("normal", nativeNormalBinding("normalView"));
    gradientInputs.set("lightDirection", inputs.get("lightDirection"));
    gradientInputs.set("builder", builder);

    const auto irradiance =
        invoke(gradientIrradiance().call({}, {gradientInputs}), "mul")({inputs.get("lightColor")});
    const auto diffuse = brdfLambert(nativePropertyNodeBinding("diffuseColor").get("rgb"));
    invoke(inputs.get("reflectedLight").get("directDiffuse"), "addAssign")(
        {invoke(irradiance, "mul")({diffuse})});
    return {};
}

// indirect( builder ) { ... }
JSValue indirect(const JSValue&, const Args& args, const Args&) {
    const auto context = arg(args, 0).get("context");
    const auto reflectedLight = context.get("reflectedLight");
    const auto diffuse = brdfLambert(nativePropertyNodeBinding("diffuseColor"));
    invoke(reflectedLight.get("indirectDiffuse"), "addAssign")(
        {invoke(context.get("irradiance"), "mul")({diffuse})});
    invoke(reflectedLight.get("indirectDiffuse"), "mulAssign")({context.get("ambientOcclusion")});
    return {};
}
}  // namespace

JSValue nativeToonLightingModelModule() {
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ToonLightingModel module");
    nativePropertyNodeModule();
    nativeNormalModule();
    nativeMathNodeModule();
    nativeTSLBase();
    nativeMaterialReferenceNodeModule();
    nativeBRDFLambertModule();

    const auto base = makeLightingModelBase();
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args&, const Args& captures) -> JSValue {
            return captures[0].get("constructor").getPrototype().construct({}, target);
        },
        {holder}, "ToonLightingModel", 0);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    ctor.setFunctionSourceText(ToonLightingModelSource::classSource);

    for (const auto& [name, arity, callback, source] : std::initializer_list<
             std::tuple<const char*, unsigned, JSValue::NativeFunction, const char*>>{
             {"direct", 2, direct, ToonLightingModelSource::directSource},
             {"indirect", 1, indirect, ToonLightingModelSource::indirectSource}}) {
        auto fn = JSValue::function(callback, {}, name, arity);
        fn.setFunctionSourceText(source);
        JSPropertyDescriptor descriptor;
        descriptor.value = fn;
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }

    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", ctor);
    auto module = JSValue::object(nullptr);
    own(module, "default", ctor);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeToonLightingModelBinding(const std::string& name) {
    const auto keeper = nativeToonLightingModelModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeToonLightingModelConstructor() {
    return nativeToonLightingModelBinding("default");
}
