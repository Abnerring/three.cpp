#include "nodes/functions/PhysicalLightingModel.h"

#include "core/JSWeakMap.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
JSWeakValue& baseKeeper() { static thread_local JSWeakValue weak; return weak; }

// Forward declarations for the shared helpers defined further down.
JSValue floatNode(double value);
JSValue vec3Node(const JSValue& x, const JSValue& y, const JSValue& z);
JSValue inputObject(const Args& args);
JSValue constants();

[[noreturn]] JSValue notPorted(const char* name) {
    throw std::runtime_error(std::string("PhysicalLightingModel: ") + name + " is not ported yet");
}

// evalSensitivity( OPD, shift ) { ... spectral sensitivity transfer function ... }
JSValue evalSensitivity(const JSValue& OPD, const JSValue& shift) {
    const auto twoPi = 2.0 * 3.14159265358979323846;
    const auto phase = invoke(OPD, "mul")({JSValue(twoPi * 1.0e-9)});
    const auto val = vec3Node(JSValue(5.4856e-13), JSValue(4.4201e-13), JSValue(5.2481e-13));
    const auto pos = vec3Node(JSValue(1.6810e+06), JSValue(1.7953e+06), JSValue(2.2084e+06));
    const auto VAR = vec3Node(JSValue(4.3278e+09), JSValue(9.3046e+09), JSValue(6.6121e+09));

    const auto phaseSquared = invoke(phase, "pow2")();
    const auto amplitude = floatNode(9.7470e-14 * std::sqrt(twoPi * 4.5282e+09));
    auto x = invoke(amplitude, "mul")(
        {invoke(invoke(invoke(phase, "mul")({JSValue(2.2399e+06)}), "add")({shift.get("x")}), "cos")()});
    x = invoke(x, "mul")(
        {nativeMathNodeBinding("exp").call({}, {invoke(phaseSquared, "mul")({JSValue(-4.5282e+09)})})});

    const auto varScaled = invoke(VAR, "mul")({JSValue(twoPi)});
    const auto varSqrt = invoke(varScaled, "sqrt")();
    auto xyz = invoke(val, "mul")({varSqrt});
    xyz = invoke(xyz, "mul")(
        {invoke(invoke(invoke(pos, "mul")({phase}), "add")({shift}), "cos")()});
    xyz = invoke(xyz, "mul")(
        {nativeMathNodeBinding("exp").call(
            {}, {invoke(invoke(phaseSquared, "negate")(), "mul")({VAR})})});

    auto combined = vec3Node(invoke(xyz.get("x"), "add")({x}), xyz.get("y"), xyz.get("z"));
    const auto normalized = invoke(combined, "div")({JSValue(1.0685e-7)});
    return invoke(constants().get("XYZ_TO_REC709"), "mul")({normalized});
}

JSWeakValue& constantsCache() { static thread_local JSWeakValue weak; return weak; }

JSValue oneVec3() { return vec3Node(JSValue(1.0), JSValue(1.0), JSValue(1.0)); }

// const XYZ_TO_REC709 = mat3( ... ); const clearcoatF0 = vec3( 0.04 ); const clearcoatF90 = float( 1 );
JSValue constants() {
    if (const auto cached = constantsCache().lock(); cached && cached->toBoolean()) return *cached;
    auto values = JSValue::object(nullptr);
    auto mat = JSValue::array(9);
    const double xyzToRec709[9] = {3.2404542, -1.5371385, -0.4985314,
                                   -0.9692660, 1.8760108, 0.0415560,
                                   0.0556434, -0.2040259, 1.0572252};
    for (std::size_t index = 0; index < 9; ++index) {
        mat.set(index, JSValue(xyzToRec709[index]));
    }
    values.set("XYZ_TO_REC709", nativeTSLBaseBinding("mat3").call({}, {mat}));
    values.set("clearcoatF0", vec3Node(JSValue(0.04), JSValue(0.04), JSValue(0.04)));
    values.set("clearcoatF90", floatNode(1.0));
    constantsCache() = JSWeakValue(values);
    return values;
}

// Fresnel0ToIor( fresnel0 ) { const sqrtF0 = fresnel0.sqrt(); return vec3(1).add(sqrtF0).div(vec3(1).sub(sqrtF0)); }
JSValue fresnel0ToIor(const JSValue& fresnel0) {
    const auto sqrtF0 = invoke(fresnel0, "sqrt")();
    const auto numerator = invoke(oneVec3(), "add")({sqrtF0});
    const auto denominator = invoke(oneVec3(), "sub")({sqrtF0});
    return invoke(numerator, "div")({denominator});
}

// IorToFresnel0( transmittedIor, incidentIor ) { return ( t - i ) / ( t + i ); } squared
JSValue iorToFresnel0(const JSValue& transmittedIor, const JSValue& incidentIor) {
    const auto numerator = invoke(transmittedIor, "sub")({incidentIor});
    const auto denominator = invoke(transmittedIor, "add")({incidentIor});
    return invoke(invoke(numerator, "div")({denominator}), "pow2")();
}

// IBLSheenBRDF = Fn( ( { normal, viewDir, roughness } ) => { ... } )
JSValue iblSheenBrdfBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = inputObject(args);
    const auto normal = inputs.get("normal");
    const auto viewDir = inputs.get("viewDir");
    const auto roughness = inputs.get("roughness");

    const auto dotNV = nativeMathNodeBinding("saturate").call({}, {invoke(normal, "dot")({viewDir})});
    const auto r2 = invoke(roughness, "mul")({roughness});
    const auto rInv = nativeMathNodeBinding("reciprocal").call(
        {}, {invoke(roughness, "add")({JSValue(0.1)})});

    JSValue a = floatNode(-1.9362);
    a = invoke(a, "add")({invoke(roughness, "mul")({JSValue(1.0678)})});
    a = invoke(a, "add")({invoke(r2, "mul")({JSValue(0.4573)})});
    a = invoke(a, "sub")({invoke(rInv, "mul")({JSValue(0.8469)})});

    JSValue b = floatNode(-0.6014);
    b = invoke(b, "add")({invoke(roughness, "mul")({JSValue(0.5538)})});
    b = invoke(b, "sub")({invoke(r2, "mul")({JSValue(0.4670)})});
    b = invoke(b, "sub")({invoke(rInv, "mul")({JSValue(0.1255)})});

    auto exponent = invoke(a, "mul")({dotNV});
    exponent = invoke(exponent, "add")({b});
    return nativeMathNodeBinding("saturate").call({}, {nativeMathNodeBinding("exp").call({}, {exponent})});
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
    if (name == "IBLSheenBRDF") return makeFn(iblSheenBrdfBody);
    if (name == "XYZ_TO_REC709") return constants().get("XYZ_TO_REC709");
    if (name == "clearcoatF0") return constants().get("clearcoatF0");
    if (name == "clearcoatF90") return constants().get("clearcoatF90");
    if (name == "evalSensitivity") {
        return JSValue::function(
            +[](const JSValue&, const Args& args, const Args&) -> JSValue {
                return evalSensitivity(arg(args, 0), arg(args, 1));
            });
    }
    if (name == "Fresnel0ToIor") {
        return JSValue::function(
            +[](const JSValue&, const Args& args, const Args&) -> JSValue {
                return fresnel0ToIor(arg(args, 0));
            });
    }
    if (name == "IorToFresnel0") {
        return JSValue::function(
            +[](const JSValue&, const Args& args, const Args&) -> JSValue {
                return iorToFresnel0(arg(args, 0), arg(args, 1));
            });
    }
    return JSValue{};
}