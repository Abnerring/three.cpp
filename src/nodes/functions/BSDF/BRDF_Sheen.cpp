#include "nodes/functions/BSDF/BRDF_Sheen.h"

#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}

JSValue dCharlieBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = arg(args, 0);
    const auto roughness = inputs.get("roughness");
    const auto dotNH = inputs.get("dotNH");
    const auto alpha = invoke(roughness, "pow2")();
    const auto invAlpha = invoke(nativeTSLBaseBinding("float").call({}, {1.0}), "div")({alpha});
    const auto cos2h = invoke(dotNH, "pow2")();
    const auto sin2h = invoke(invoke(cos2h, "oneMinus")(), "max")({0.0078125});
    const auto exponent = invoke(invAlpha, "mul")({0.5});
    const auto numerator = invoke(invoke(nativeTSLBaseBinding("float").call({}, {2.0}), "add")({invAlpha}), "mul")({invoke(sin2h, "pow")({exponent})});
    return invoke(numerator, "div")({2.0 * 3.141592653589793238462643383279502884});
}

JSValue vNeubeltBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = arg(args, 0);
    const auto dotNV = inputs.get("dotNV");
    const auto dotNL = inputs.get("dotNL");
    const auto sum = invoke(invoke(dotNL, "add")({dotNV}), "sub")({invoke(dotNL, "mul")({dotNV})});
    const auto denominator = invoke(nativeTSLBaseBinding("float").call({}, {4.0}), "mul")({sum});
    return invoke(invoke(nativeTSLBaseBinding("float").call({}, {1.0}), "div")({denominator}), "clamp")();
}

JSValue body(const JSValue&, const Args& args, const Args& captures) {
    const auto inputs = arg(args, 0);
    const auto lightDirection = inputs.get("lightDirection");
    const auto positionViewDirection = captures[0];
    const auto normalView = captures[1];
    const auto sheen = captures[2];
    const auto sheenRoughness = captures[3];
    const auto dCharlie = captures[4];
    const auto vNeubelt = captures[5];

    const auto halfDir = invoke(invoke(lightDirection, "add")({positionViewDirection}), "normalize")();
    const auto dotNL = invoke(invoke(normalView, "dot")({lightDirection}), "clamp")();
    const auto dotNV = invoke(invoke(normalView, "dot")({positionViewDirection}), "clamp")();
    const auto dotNH = invoke(invoke(normalView, "dot")({halfDir}), "clamp")();
    auto dInputs = JSValue::object();
    dInputs.set("roughness", sheenRoughness);
    dInputs.set("dotNH", dotNH);
    auto vInputs = JSValue::object();
    vInputs.set("dotNV", dotNV);
    vInputs.set("dotNL", dotNL);
    const auto d = dCharlie.call({}, {dInputs});
    const auto v = vNeubelt.call({}, {vInputs});
    return invoke(invoke(sheen, "mul")({d}), "mul")({v});
}

} // namespace

JSValue nativeBRDFSheenModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "BRDF_Sheen module");

    auto dCallback = JSValue::function(dCharlieBody, {}, "", 1);
    dCallback.setFunctionSourceText("( { roughness, dotNH } ) => {\r\n\r\n\tconst alpha = roughness.pow2();\r\n\r\n\t// Estevez and Kulla 2017, \"Production Friendly Microfacet Sheen BRDF\"\r\n\tconst invAlpha = float( 1.0 ).div( alpha );\r\n\tconst cos2h = dotNH.pow2();\r\n\tconst sin2h = cos2h.oneMinus().max( 0.0078125 ); // 2^(-14/2), so sin2h^2 > 0 in fp16\r\n\r\n\treturn float( 2.0 ).add( invAlpha ).mul( sin2h.pow( invAlpha.mul( 0.5 ) ) ).div( 2.0 * Math.PI );\r\n\r\n}");
    auto dLayout = JSValue::object();
    own(dLayout, "roughness", "float");
    own(dLayout, "dotNH", "float");
    own(dLayout, "return", "float");
    const auto dCharlie = nativeTSLBaseBinding("Fn").call({}, {dCallback, dLayout});

    auto vCallback = JSValue::function(vNeubeltBody, {}, "", 1);
    vCallback.setFunctionSourceText("( { dotNV, dotNL } ) => {\r\n\r\n\t// Neubelt and Pettineo 2013, \"Crafting a Next-gen Material Pipeline for The Order: 1886\"\r\n\treturn float( 1.0 ).div( float( 4.0 ).mul( dotNL.add( dotNV ).sub( dotNL.mul( dotNV ) ) ) ).clamp();\r\n\r\n}");
    auto vLayout = JSValue::object();
    own(vLayout, "dotNV", "float");
    own(vLayout, "dotNL", "float");
    own(vLayout, "return", "float");
    const auto vNeubelt = nativeTSLBaseBinding("Fn").call({}, {vCallback, vLayout});

    const auto normalModule = nativeNormalModule();
    const auto positionModule = nativePositionModule();
    const auto propertyModule = nativePropertyNodeModule();
    (void)normalModule;
    (void)positionModule;
    (void)propertyModule;
    auto callback = JSValue::function(body, {
        nativePositionBinding("positionViewDirection"), nativeNormalBinding("normalView"),
        nativePropertyNodeBinding("sheen"), nativePropertyNodeBinding("sheenRoughness"),
        dCharlie, vNeubelt
    }, "", 1);
    callback.setFunctionSourceText("( { lightDirection } ) => {\r\n\r\n\tconst halfDir = lightDirection.add( positionViewDirection ).normalize();\r\n\r\n\tconst dotNL = normalView.dot( lightDirection ).clamp();\r\n\tconst dotNV = normalView.dot( positionViewDirection ).clamp();\r\n\tconst dotNH = normalView.dot( halfDir ).clamp();\r\n\r\n\tconst D = D_Charlie( { roughness: sheenRoughness, dotNH } );\r\n\tconst V = V_Neubelt( { dotNV, dotNL } );\r\n\r\n\treturn sheen.mul( D ).mul( V );\r\n\r\n}");
    const auto value = nativeTSLBaseBinding("Fn").call({}, {callback});

    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", value);
    auto module = JSValue::object(nullptr);
    own(module, "default", value);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeBRDFSheenBinding(const std::string& name) {
    const auto keeper = nativeBRDFSheenModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeBRDFSheen() {
    return nativeBRDFSheenBinding("default");
}
