#include "nodes/functions/BSDF/V_GGX_SmithCorrelated_Anisotropic.h"

#include "nodes/math/MathNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}

JSValue body(const JSValue&, const Args& args, const Args&) {
    const auto inputs = arg(args, 0);
    const auto alphaT = inputs.get("alphaT");
    const auto alphaB = inputs.get("alphaB");
    const auto dotTV = inputs.get("dotTV");
    const auto dotBV = inputs.get("dotBV");
    const auto dotTL = inputs.get("dotTL");
    const auto dotBL = inputs.get("dotBL");
    const auto dotNV = inputs.get("dotNV");
    const auto dotNL = inputs.get("dotNL");
    const auto vec3 = nativeTSLBaseBinding("vec3");
    const auto gvVector = vec3.call({}, {
        invoke(alphaT, "mul")({dotTV}),
        invoke(alphaB, "mul")({dotBV}),
        dotNV
    });
    const auto glVector = vec3.call({}, {
        invoke(alphaT, "mul")({dotTL}),
        invoke(alphaB, "mul")({dotBL}),
        dotNL
    });
    const auto gv = invoke(dotNL, "mul")({invoke(gvVector, "length")()});
    const auto gl = invoke(dotNV, "mul")({invoke(glVector, "length")()});
    const auto denominator = invoke(invoke(gv, "add")({gl}), "max")({nativeMathNodeBinding("EPSILON")});
    return nativeOperatorNodeBinding("div").call({}, {0.5, denominator});
}
} // namespace

JSValue nativeVGGXSmithCorrelatedAnisotropicModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "V_GGX_SmithCorrelated_Anisotropic module");
    auto callback = JSValue::function(body, {}, "", 1);
    callback.setFunctionSourceText("( { alphaT, alphaB, dotTV, dotBV, dotTL, dotBL, dotNV, dotNL } ) => {\r\n\r\n\tconst gv = dotNL.mul( vec3( alphaT.mul( dotTV ), alphaB.mul( dotBV ), dotNV ).length() );\r\n\tconst gl = dotNV.mul( vec3( alphaT.mul( dotTL ), alphaB.mul( dotBL ), dotNL ).length() );\r\n\r\n\treturn div( 0.5, gv.add( gl ).max( EPSILON ) );\r\n\r\n}");
    auto layout = JSValue::object();
    for (const auto* name : {"alphaT", "alphaB", "dotTV", "dotBV", "dotTL", "dotBL", "dotNV", "dotNL"}) own(layout, name, "float");
    own(layout, "return", "float");
    const auto value = nativeTSLBaseBinding("Fn").call({}, {callback, layout});

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

JSValue nativeVGGXSmithCorrelatedAnisotropicBinding(const std::string& name) {
    const auto keeper = nativeVGGXSmithCorrelatedAnisotropicModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeVGGXSmithCorrelatedAnisotropic() {
    return nativeVGGXSmithCorrelatedAnisotropicBinding("default");
}
