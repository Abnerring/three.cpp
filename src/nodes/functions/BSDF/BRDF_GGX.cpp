#include "nodes/functions/BSDF/BRDF_GGX.h"

#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/functions/BSDF/D_GGX.h"
#include "nodes/functions/BSDF/D_GGX_Anisotropic.h"
#include "nodes/functions/BSDF/F_Schlick.h"
#include "nodes/functions/BSDF/V_GGX_SmithCorrelated.h"
#include "nodes/functions/BSDF/V_GGX_SmithCorrelated_Anisotropic.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}

JSValue body(const JSValue&, const Args& args, const Args& captures) {
    const auto inputs = arg(args, 0);
    const auto lightDirection = inputs.get("lightDirection");
    const auto f0 = inputs.get("f0");
    const auto f90 = inputs.get("f90");
    const auto roughness = inputs.get("roughness");
    const auto f = inputs.get("f");
    auto normalView = inputs.get("normalView");
    if (normalView.isUndefined()) normalView = captures[0];
    auto viewDirection = inputs.get("viewDirection");
    if (viewDirection.isUndefined()) viewDirection = captures[1];

    const auto alpha = invoke(invoke(roughness, "max")({0.045}), "pow2")();
    const auto halfDir = invoke(invoke(lightDirection, "add")({viewDirection}), "normalize")();
    const auto dotNL = invoke(invoke(normalView, "dot")({lightDirection}), "clamp")();
    const auto dotNV = invoke(invoke(normalView, "dot")({viewDirection}), "clamp")();
    const auto dotNH = invoke(invoke(normalView, "dot")({halfDir}), "clamp")();
    const auto dotVH = invoke(invoke(viewDirection, "dot")({halfDir}), "clamp")();

    auto fresnelInputs = JSValue::object();
    fresnelInputs.set("f0", f0);
    fresnelInputs.set("f90", f90);
    fresnelInputs.set("dotVH", dotVH);
    auto F = captures[6].call({}, {fresnelInputs});

    const auto defined = nativeTSLBaseBinding("defined");
    if (defined.call({}, {inputs.get("USE_IRIDESCENCE")}).toBoolean()) {
        F = invoke(captures[2], "mix")({F, f});
    }

    JSValue V;
    JSValue D;
    if (defined.call({}, {inputs.get("USE_ANISOTROPY")}).toBoolean()) {
        const auto anisotropyT = captures[4];
        const auto anisotropyB = captures[5];
        const auto dotTL = invoke(anisotropyT, "dot")({lightDirection});
        const auto dotTV = invoke(anisotropyT, "dot")({viewDirection});
        const auto dotTH = invoke(anisotropyT, "dot")({halfDir});
        const auto dotBL = invoke(anisotropyB, "dot")({lightDirection});
        const auto dotBV = invoke(anisotropyB, "dot")({viewDirection});
        const auto dotBH = invoke(anisotropyB, "dot")({halfDir});
        const auto clampedAlphaT = invoke(captures[3], "max")({alpha});

        auto vInputs = JSValue::object();
        vInputs.set("alphaT", clampedAlphaT);
        vInputs.set("alphaB", alpha);
        vInputs.set("dotTV", dotTV);
        vInputs.set("dotBV", dotBV);
        vInputs.set("dotTL", dotTL);
        vInputs.set("dotBL", dotBL);
        vInputs.set("dotNV", dotNV);
        vInputs.set("dotNL", dotNL);
        V = captures[8].call({}, {vInputs});

        auto dInputs = JSValue::object();
        dInputs.set("alphaT", clampedAlphaT);
        dInputs.set("alphaB", alpha);
        dInputs.set("dotNH", dotNH);
        dInputs.set("dotTH", dotTH);
        dInputs.set("dotBH", dotBH);
        D = captures[10].call({}, {dInputs});
    } else {
        auto vInputs = JSValue::object();
        vInputs.set("alpha", alpha);
        vInputs.set("dotNL", dotNL);
        vInputs.set("dotNV", dotNV);
        V = captures[7].call({}, {vInputs});

        auto dInputs = JSValue::object();
        dInputs.set("alpha", alpha);
        dInputs.set("dotNH", dotNH);
        D = captures[9].call({}, {dInputs});
    }

    return invoke(invoke(F, "mul")({V}), "mul")({D});
}
} // namespace

JSValue nativeBRDFGGXModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "BRDF_GGX module");
    nativeNormalModule();
    nativePositionModule();
    nativePropertyNodeModule();
    nativeFSchlickModule();
    nativeVGGXSmithCorrelatedModule();
    nativeVGGXSmithCorrelatedAnisotropicModule();
    nativeDGGXModule();
    nativeDGGXAnisotropicModule();

    auto callback = JSValue::function(body, {
        nativeNormalBinding("normalView"), nativePositionBinding("positionViewDirection"),
        nativePropertyNodeBinding("iridescence"), nativePropertyNodeBinding("alphaT"),
        nativePropertyNodeBinding("anisotropyT"), nativePropertyNodeBinding("anisotropyB"),
        nativeFSchlick(), nativeVGGXSmithCorrelated(), nativeVGGXSmithCorrelatedAnisotropic(),
        nativeDGGX(), nativeDGGXAnisotropic()
    }, "", 1);
    callback.setFunctionSourceText("( { lightDirection, f0, f90, roughness, f, normalView = NormalView, viewDirection = positionViewDirection, USE_IRIDESCENCE, USE_ANISOTROPY } ) => {\r\n\r\n\tconst alpha = roughness.max( 0.045 ).pow2(); // punctual lights need a minimum roughness to show a highlight\r\n\r\n\tconst halfDir = lightDirection.add( viewDirection ).normalize();\r\n\r\n\tconst dotNL = normalView.dot( lightDirection ).clamp();\r\n\tconst dotNV = normalView.dot( viewDirection ).clamp(); // @ TODO: Move to core dotNV\r\n\tconst dotNH = normalView.dot( halfDir ).clamp();\r\n\tconst dotVH = viewDirection.dot( halfDir ).clamp();\r\n\r\n\tlet F = F_Schlick( { f0, f90, dotVH } );\r\n\tlet V, D;\r\n\r\n\tif ( defined( USE_IRIDESCENCE ) ) {\r\n\r\n\t\tF = iridescence.mix( F, f );\r\n\r\n\t}\r\n\r\n\tif ( defined( USE_ANISOTROPY ) ) {\r\n\r\n\t\tconst dotTL = anisotropyT.dot( lightDirection );\r\n\t\tconst dotTV = anisotropyT.dot( viewDirection );\r\n\t\tconst dotTH = anisotropyT.dot( halfDir );\r\n\t\tconst dotBL = anisotropyB.dot( lightDirection );\r\n\t\tconst dotBV = anisotropyB.dot( viewDirection );\r\n\t\tconst dotBH = anisotropyB.dot( halfDir );\r\n\r\n\t\tconst clampedAlphaT = alphaT.max( alpha );\r\n\r\n\t\tV = V_GGX_SmithCorrelated_Anisotropic( { alphaT: clampedAlphaT, alphaB: alpha, dotTV, dotBV, dotTL, dotBL, dotNV, dotNL } );\r\n\t\tD = D_GGX_Anisotropic( { alphaT: clampedAlphaT, alphaB: alpha, dotNH, dotTH, dotBH } );\r\n\r\n\t} else {\r\n\r\n\t\tV = V_GGX_SmithCorrelated( { alpha, dotNL, dotNV } );\r\n\t\tD = D_GGX( { alpha, dotNH } );\r\n\r\n\t}\r\n\r\n\treturn F.mul( V ).mul( D );\r\n\r\n}");
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

JSValue nativeBRDFGGXBinding(const std::string& name) {
    const auto keeper = nativeBRDFGGXModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeBRDFGGX() {
    return nativeBRDFGGXBinding("default");
}
