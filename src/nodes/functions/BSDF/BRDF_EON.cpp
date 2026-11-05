#include "nodes/functions/BSDF/BRDF_EON.h"

#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/functions/BSDF/BRDF_EONSources.inc"

namespace {
using namespace TSLFunctionRuntime;

constexpr double EON_EPSILON = 1.0e-7;
constexpr double FON_A_COEFFICIENT = 0.5 - 2.0 / (3.0 * 3.141592653589793238462643383279502884);
constexpr double FON_AVERAGE_ALBEDO_COEFFICIENT = 2.0 / 3.0 - 28.0 / (15.0 * 3.141592653589793238462643383279502884);

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}

JSValue directionalBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = arg(args, 0);
    const auto mu = inputs.get("mu");
    const auto roughness = inputs.get("roughness");
    const auto A = inputs.get("A");
    const auto muComp = invoke(mu, "oneMinus")();
    auto g = invoke(muComp, "mul")({0.0714429953});
    g = invoke(g, "sub")({0.332181442});
    g = invoke(muComp, "mul")({g});
    g = invoke(g, "add")({0.491881867});
    g = invoke(muComp, "mul")({g});
    g = invoke(g, "add")({0.0571085289});
    g = invoke(muComp, "mul")({g});
    const auto value = invoke(roughness, "mul")({g});
    return invoke(A, "mul")({invoke(value, "add")({1.0})});
}

JSValue eonDirectionalBody(const JSValue&, const Args& args, const Args& captures) {
    const auto inputs = arg(args, 0);
    const auto diffuseColor = inputs.get("diffuseColor");
    const auto roughness = inputs.get("roughness");
    const auto dotNV = inputs.get("dotNV");
    const auto rho = invoke(diffuseColor, "clamp")();
    const auto A = invoke(invoke(invoke(roughness, "mul")({FON_A_COEFFICIENT}), "add")({1.0}), "reciprocal")();
    auto directionalInputs = JSValue::object();
    own(directionalInputs, "mu", invoke(dotNV, "clamp")());
    own(directionalInputs, "roughness", roughness);
    own(directionalInputs, "A", A);
    const auto directionalAlbedo = captures[0].call({}, {directionalInputs});
    const auto averageAlbedo = invoke(A, "mul")(
        {invoke(invoke(roughness, "mul")({FON_AVERAGE_ALBEDO_COEFFICIENT}), "add")({1.0})});
    const auto divide = invoke(invoke(rho, "mul")({rho, averageAlbedo}), "div");
    const auto rhoMultiScatter = divide(
        {invoke(invoke(invoke(rho, "mul")({invoke(averageAlbedo, "oneMinus")()}), "oneMinus")(), "max")({EON_EPSILON})});
    const auto add = invoke(invoke(rho, "mul")({directionalAlbedo}), "add");
    const auto eonAlbedo = add({invoke(rhoMultiScatter, "mul")({invoke(directionalAlbedo, "oneMinus")()})});
    return invoke(invoke(roughness, "lessThanEqual")({EON_EPSILON}), "select")({rho, eonAlbedo});
}

JSValue body(const JSValue&, const Args& args, const Args& captures) {
    const auto inputs = arg(args, 0);
    const auto lightDirection = inputs.get("lightDirection");
    const auto diffuseColor = inputs.get("diffuseColor");
    const auto roughness = inputs.get("roughness");
    auto normalView = inputs.get("normalView");
    if (normalView.isUndefined()) normalView = captures[0];
    auto viewDirection = inputs.get("viewDirection");
    if (viewDirection.isUndefined()) viewDirection = captures[1];

    const auto rho = invoke(diffuseColor, "clamp")();
    const auto dotNL = invoke(invoke(normalView, "dot")({lightDirection}), "clamp")();
    const auto dotNV = invoke(invoke(normalView, "dot")({viewDirection}), "clamp")();
    const auto s = invoke(invoke(lightDirection, "dot")({viewDirection}), "sub")({invoke(dotNL, "mul")({dotNV})});
    const auto selectRatio = invoke(invoke(s, "greaterThan")({0.0}), "select");
    const auto divideS = invoke(s, "div");
    const auto denominator = invoke(invoke(dotNL, "max")({dotNV}), "max")({EON_EPSILON});
    const auto sOverT = selectRatio({divideS({denominator}), s});

    const auto A = invoke(invoke(roughness, "mul")({FON_A_COEFFICIENT}), "add")({1.0});
    const auto reciprocalA = invoke(A, "reciprocal")();
    const auto singleMultiply = invoke(rho, "mul");
    const auto singleScale = invoke(roughness, "mul")({sOverT});
    const auto singleScatter = singleMultiply({1.0 / 3.141592653589793238462643383279502884,
                                                     reciprocalA,
                                                     invoke(singleScale, "add")({1.0})});

    const auto averageAlbedo = invoke(reciprocalA, "mul")({invoke(invoke(roughness, "mul")({FON_AVERAGE_ALBEDO_COEFFICIENT}), "add")({1.0})});
    const auto directionalInputs = [&](const JSValue& mu) {
        auto inputs = JSValue::object();
        own(inputs, "mu", mu); own(inputs, "roughness", roughness); own(inputs, "A", reciprocalA);
        return inputs;
    };
    const auto albedoV = captures[2].call({}, {directionalInputs(dotNV)});
    const auto albedoL = captures[2].call({}, {directionalInputs(dotNL)});

    const auto rhoAverage = invoke(rho, "mul")({rho, averageAlbedo});
    const auto divideRho = invoke(rhoAverage, "div");
    const auto rhoDenominator = invoke(invoke(rho, "mul")({invoke(averageAlbedo, "oneMinus")()}), "oneMinus")();
    const auto rhoMultiScatter = divideRho({invoke(rhoDenominator, "max")({EON_EPSILON})});
    const auto multiMultiply = invoke(rhoMultiScatter, "mul");
    const auto albedoVTerm = invoke(invoke(albedoV, "oneMinus")(), "max")({EON_EPSILON});
    const auto albedoLTerm = invoke(invoke(albedoL, "oneMinus")(), "max")({EON_EPSILON});
    const auto multiNumerator = multiMultiply({1.0 / 3.141592653589793238462643383279502884, albedoVTerm, albedoLTerm});
    const auto multiScatter = invoke(multiNumerator, "div")({invoke(invoke(averageAlbedo, "oneMinus")(), "max")({EON_EPSILON})});
    const auto eon = invoke(singleScatter, "add")({multiScatter});
    return invoke(invoke(roughness, "lessThanEqual")({EON_EPSILON}), "select")({invoke(rho, "mul")({1.0 / 3.141592653589793238462643383279502884}), eon});
}
} // namespace

JSValue nativeBRDFEONModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "BRDF_EON module");

    auto directionalCallback = JSValue::function(directionalBody, {}, "", 1);
    directionalCallback.setFunctionSourceText(BRDFEONSource::directionalSource);
    auto layout = JSValue::object();
    own(layout, "mu", "float"); own(layout, "roughness", "float"); own(layout, "A", "float"); own(layout, "return", "float");
    const auto directional = nativeTSLBaseBinding("Fn").call({}, {directionalCallback, layout});

    const auto normal = nativeNormalModule();
    const auto position = nativePositionModule();
    (void)normal; (void)position;
    auto callback = JSValue::function(body, {nativeNormalBinding("normalView"), nativePositionBinding("positionViewDirection"), directional}, "", 1);
    callback.setFunctionSourceText(BRDFEONSource::brdfSource);
    const auto value = nativeTSLBaseBinding("Fn").call({}, {callback});
    auto eonCallback = JSValue::function(eonDirectionalBody, {directional}, "", 1);
    eonCallback.setFunctionSourceText(BRDFEONSource::eonDirectionalSource);
    auto eonLayout = JSValue::object();
    own(eonLayout, "diffuseColor", "vec3"); own(eonLayout, "roughness", "float");
    own(eonLayout, "dotNV", "float"); own(eonLayout, "return", "vec3");
    const auto eonDirectional = nativeTSLBaseBinding("Fn").call({}, {eonCallback, eonLayout});
    auto bindings = JSValue::object(nullptr);
    own(bindings, "EON_DirectionalAlbedo", eonDirectional); own(bindings, "default", value);
    auto module = JSValue::object(nullptr);
    own(module, "EON_DirectionalAlbedo", eonDirectional); own(module, "default", value);
    bindings.retainForHeapLifetime(); lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime(); weak = JSWeakValue(module); return module;
}

JSValue nativeBRDFEONBinding(const std::string& name) {
    const auto keeper = nativeBRDFEONModule(); (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeBRDFEON() { return nativeBRDFEONBinding("default"); }
JSValue nativeEONDirectionalAlbedo() { return nativeBRDFEONBinding("EON_DirectionalAlbedo"); }
