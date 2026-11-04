#include "nodes/functions/BSDF/BRDF_EON.h"

#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

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
    g = invoke(muComp, "mul")({g});
    const auto value = invoke(roughness, "mul")({g});
    return invoke(A, "mul")({invoke(value, "add")({1.0})});
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
    const auto denominator = invoke(invoke(dotNL, "max")({dotNV}), "max")({EON_EPSILON});
    const auto ratio = invoke(s, "div")({denominator});
    const auto sOverT = invoke(invoke(s, "greaterThan")({0.0}), "select")({ratio, s});

    const auto A = invoke(invoke(roughness, "mul")({FON_A_COEFFICIENT}), "add")({1.0});
    const auto reciprocalA = invoke(A, "reciprocal")();
    const auto singleScale = invoke(roughness, "mul")({sOverT});
    const auto singleScatter = invoke(rho, "mul")({1.0 / 3.141592653589793238462643383279502884,
                                                     reciprocalA,
                                                     invoke(singleScale, "add")({1.0})});

    const auto averageAlbedo = invoke(reciprocalA, "mul")({invoke(invoke(roughness, "mul")({FON_AVERAGE_ALBEDO_COEFFICIENT}), "add")({1.0})});
    auto directionalInputs = JSValue::object();
    directionalInputs.set("roughness", roughness);
    directionalInputs.set("A", reciprocalA);
    directionalInputs.set("mu", dotNV);
    const auto albedoV = captures[2].call({}, {directionalInputs});
    directionalInputs.set("mu", dotNL);
    const auto albedoL = captures[2].call({}, {directionalInputs});

    const auto rhoAverage = invoke(rho, "mul")({rho, averageAlbedo});
    const auto rhoDenominator = invoke(invoke(rho, "mul")({invoke(averageAlbedo, "oneMinus")()}), "oneMinus")();
    const auto rhoMultiScatter = invoke(rhoAverage, "div")({invoke(rhoDenominator, "max")({EON_EPSILON})});
    const auto albedoVTerm = invoke(invoke(albedoV, "oneMinus")(), "max")({EON_EPSILON});
    const auto albedoLTerm = invoke(invoke(albedoL, "oneMinus")(), "max")({EON_EPSILON});
    const auto multiNumerator = invoke(rhoMultiScatter, "mul")({1.0 / 3.141592653589793238462643383279502884, albedoVTerm, albedoLTerm});
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
    directionalCallback.setFunctionSourceText("( { mu, roughness, A } ) => {\r\n\r\n\tconst muComp = mu.oneMinus();\r\n\tconst gOverPi = muComp.mul(\r\n\t\tmuComp.mul(\r\n\t\t\tmuComp.mul(\r\n\t\t\t\tmuComp.mul(\r\n\t\t\t\t\tmuComp.mul( 0.0714429953 ).sub( 0.332181442 )\r\n\t\t\t\t).add( 0.491881867 )\r\n\t\t\t).add( 0.0571085289 )\r\n\t\t)\r\n\t);\r\n\r\n\treturn A.mul( roughness.mul( gOverPi ).add( 1.0 ) );\r\n\r\n}");
    auto layout = JSValue::object();
    own(layout, "mu", "float"); own(layout, "roughness", "float"); own(layout, "A", "float"); own(layout, "return", "float");
    const auto directional = nativeTSLBaseBinding("Fn").call({}, {directionalCallback, layout});

    const auto normal = nativeNormalModule();
    const auto position = nativePositionModule();
    (void)normal; (void)position;
    auto callback = JSValue::function(body, {nativeNormalBinding("normalView"), nativePositionBinding("positionViewDirection"), directional}, "", 1);
    callback.setFunctionSourceText("( { lightDirection, diffuseColor, roughness, normalView = NormalView, viewDirection = positionViewDirection } ) => {\r\n\r\n\tconst rho = diffuseColor.clamp();\r\n\tconst dotNL = normalView.dot( lightDirection ).clamp();\r\n\tconst dotNV = normalView.dot( viewDirection ).clamp();\r\n\tconst s = lightDirection.dot( viewDirection ).sub( dotNL.mul( dotNV ) );\r\n\tconst sOverT = s.greaterThan( 0.0 ).select( s.div( dotNL.max( dotNV ).max( EON_EPSILON ) ), s );\r\n\r\n\tconst A = roughness.mul( FON_A_COEFFICIENT ).add( 1.0 ).reciprocal();\r\n\tconst singleScatter = rho.mul( 1 / Math.PI, A, roughness.mul( sOverT ).add( 1.0 ) );\r\n\r\n\tconst averageAlbedo = A.mul( roughness.mul( FON_AVERAGE_ALBEDO_COEFFICIENT ).add( 1.0 ) );\r\n\tconst albedoV = FON_DirectionalAlbedo( { mu: dotNV, roughness, A } );\r\n\tconst albedoL = FON_DirectionalAlbedo( { mu: dotNL, roughness, A } );\r\n\tconst rhoMultiScatter = rho.mul( rho, averageAlbedo ).div( rho.mul( averageAlbedo.oneMinus() ).oneMinus().max( EON_EPSILON ) );\r\n\tconst multiScatter = rhoMultiScatter.mul(\r\n\t\t1 / Math.PI,\r\n\t\talbedoV.oneMinus().max( EON_EPSILON ),\r\n\t\talbedoL.oneMinus().max( EON_EPSILON )\r\n\t).div( averageAlbedo.oneMinus().max( EON_EPSILON ) );\r\n\tconst eon = singleScatter.add( multiScatter );\r\n\r\n\treturn roughness.lessThanEqual( EON_EPSILON ).select( rho.mul( 1 / Math.PI ), eon );\r\n\r\n}");
    const auto value = nativeTSLBaseBinding("Fn").call({}, {callback});
    auto bindings = JSValue::object(nullptr); own(bindings, "default", value);
    auto module = JSValue::object(nullptr); own(module, "default", value);
    bindings.retainForHeapLifetime(); lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime(); weak = JSWeakValue(module); return module;
}

JSValue nativeBRDFEONBinding(const std::string& name) {
    const auto keeper = nativeBRDFEONModule(); (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeBRDFEON() { return nativeBRDFEONBinding("default"); }
