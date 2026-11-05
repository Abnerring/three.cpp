#include "nodes/functions/PhongLightingModel.h"

#include "nodes/accessors/MaterialNode.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/functions/BSDF/BRDF_Lambert.h"
#include "nodes/functions/BSDF/F_Schlick.h"
#include "nodes/functions/BasicLightingModel.h"
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

JSValue floatNode(double value) {
    return nativeTSLBaseBinding("float").call({}, {JSValue(value)});
}

// const BRDF_BlinnPhong = Fn( ( { lightDirection } ) => { ... } )
JSValue brdfBlinnPhongBody(const JSValue&, const Args& args, const Args&) {
    auto iterator = getJSIterator(arg(args, 0));
    JSValue inputs;
    if (!iterator.done) {
        const auto value = stepJSIterator(iterator);
        if (value) inputs = *value;
    }
    if (!iterator.done) closeJSIterator(iterator);

    const auto lightDirection = inputs.get("lightDirection");
    const auto normalView = nativeNormalBinding("normalView");
    const auto positionViewDirection = nativePositionBinding("positionViewDirection");

    // const halfDir = lightDirection.add( positionViewDirection ).normalize();
    const auto halfDir =
        invoke(invoke(lightDirection, "add")({positionViewDirection}), "normalize")();
    const auto dotNH = invoke(invoke(normalView, "dot")({halfDir}), "clamp")();
    const auto dotVH = invoke(invoke(positionViewDirection, "dot")({halfDir}), "clamp")();

    // F_Schlick( { f0: specularColor, f90: 1.0, dotVH } )
    auto fresnelInputs = JSValue::object(nullptr);
    fresnelInputs.set("f0", nativePropertyNodeBinding("specularColor"));
    fresnelInputs.set("f90", JSValue(1.0));
    fresnelInputs.set("dotVH", dotVH);
    const auto fresnel = nativeFSchlick().call({}, {fresnelInputs});

    // const G = float( 0.25 ); const D = shininess.mul(0.5).add(1.0).mul(1/pi).mul( dotNH.pow( shininess ) )
    const auto shininess = nativePropertyNodeBinding("shininess");
    const auto distribution = invoke(
        invoke(invoke(invoke(shininess, "mul")({floatNode(0.5)}), "add")({floatNode(1.0)}),
               "mul")({floatNode(1.0 / 3.141592653589793)}),
        "mul")({invoke(dotNH, "pow")({shininess})});
    return invoke(invoke(fresnel, "mul")({floatNode(0.25)}), "mul")({distribution});
}

JSValue brdfBlinnPhong() {
    static thread_local JSWeakValue cached;
    if (const auto value = cached.lock(); value && value->isCallable()) return *value;
    auto callback = JSValue::function(brdfBlinnPhongBody, {}, "", 0);
    const auto fn = nativeTSLCoreBinding("Fn").call({}, {callback});
    cached = JSWeakValue(fn);
    return fn;
}

JSValue brdfLambert(const JSValue& diffuse) {
    auto inputs = JSValue::object(nullptr);
    inputs.set("diffuseColor", diffuse);
    return nativeBRDFLambert().call({}, {inputs});
}

// direct( { lightDirection, lightColor, reflectedLight } ) { ... }
JSValue direct(const JSValue& self, const Args& args, const Args&) {
    const auto inputs = arg(args, 0);
    const auto reflectedLight = inputs.get("reflectedLight");

    const auto dotNL = invoke(invoke(nativeNormalBinding("normalView"), "dot")({inputs.get("lightDirection")}),
                              "clamp")();
    const auto irradiance = invoke(dotNL, "mul")({inputs.get("lightColor")});
    invoke(reflectedLight.get("directDiffuse"), "addAssign")({invoke(irradiance, "mul")(
        {brdfLambert(nativePropertyNodeBinding("diffuseColor").get("rgb"))})});

    if (self.get("specular").strictEquals(true)) {
        auto blinnInputs = JSValue::object(nullptr);
        blinnInputs.set("lightDirection", inputs.get("lightDirection"));
        const auto specular = invoke(
            invoke(irradiance, "mul")({brdfBlinnPhong().call({}, {blinnInputs})}),
            "mul")({nativeMaterialNodeBinding("materialSpecularStrength")});
        invoke(reflectedLight.get("directSpecular"), "addAssign")({specular});
    }
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

JSValue nativePhongLightingModelModule() {
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "PhongLightingModel module");
    nativeBasicLightingModelModule();
    nativePropertyNodeModule();
    nativeNormalModule();
    nativePositionModule();
    nativeMathNodeModule();
    nativeTSLBase();
    nativeBRDFLambertModule();
    nativeFSchlickModule();

    const auto base = nativeBasicLightingModel();
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = captures[0].construct({}, target);
            // constructor( specular = true )
            const auto specular = arg(args, 0, JSValue{});
            self.set("specular", specular.isUndefined() ? JSValue(true) : specular);
            return self;
        },
        {base}, "PhongLightingModel", 0);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    ctor.setFunctionSourceText(PhongLightingModelSource::classSource);

    for (const auto& [name, arity, callback, source] : std::initializer_list<
             std::tuple<const char*, unsigned, JSValue::NativeFunction, const char*>>{
             {// The source declares direct with the destructured inputs only, no builder, so its length is one.
             "direct", 1, direct, PhongLightingModelSource::directSource},
             {"indirect", 1, indirect, PhongLightingModelSource::indirectSource}}) {
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

JSValue nativePhongLightingModelBinding(const std::string& name) {
    const auto keeper = nativePhongLightingModelModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativePhongLightingModelConstructor() {
    return nativePhongLightingModelBinding("default");
}
