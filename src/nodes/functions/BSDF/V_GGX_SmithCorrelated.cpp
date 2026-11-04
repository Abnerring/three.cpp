#include "nodes/functions/BSDF/V_GGX_SmithCorrelated.h"

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
    const auto alpha = inputs.get("alpha");
    const auto dotNL = inputs.get("dotNL");
    const auto dotNV = inputs.get("dotNV");

    const auto a2 = invoke(alpha, "pow2")();
    const auto a2OneMinus = invoke(a2, "oneMinus")();
    const auto gvRoot = invoke(a2, "add")({invoke(a2OneMinus, "mul")({invoke(dotNV, "pow2")()})});
    const auto glRoot = invoke(a2, "add")({invoke(a2OneMinus, "mul")({invoke(dotNL, "pow2")()})});
    const auto gv = invoke(dotNL, "mul")({invoke(gvRoot, "sqrt")()});
    const auto gl = invoke(dotNV, "mul")({invoke(glRoot, "sqrt")()});
    const auto denominator = invoke(invoke(gv, "add")({gl}), "max")({nativeMathNodeBinding("EPSILON")});
    return nativeOperatorNodeBinding("div").call({}, {0.5, denominator});
}
} // namespace

JSValue nativeVGGXSmithCorrelatedModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "V_GGX_SmithCorrelated module");
    auto callback = JSValue::function(body, {}, "", 1);
    callback.setFunctionSourceText("( { alpha, dotNL, dotNV } ) => {\r\n\r\n\tconst a2 = alpha.pow2();\r\n\r\n\tconst gv = dotNL.mul( a2.add( a2.oneMinus().mul( dotNV.pow2() ) ).sqrt() );\r\n\tconst gl = dotNV.mul( a2.add( a2.oneMinus().mul( dotNL.pow2() ) ).sqrt() );\r\n\r\n\treturn div( 0.5, gv.add( gl ).max( EPSILON ) );\r\n\r\n}");
    auto layout = JSValue::object();
    own(layout, "alpha", "float");
    own(layout, "dotNL", "float");
    own(layout, "dotNV", "float");
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

JSValue nativeVGGXSmithCorrelatedBinding(const std::string& name) {
    const auto keeper = nativeVGGXSmithCorrelatedModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeVGGXSmithCorrelated() {
    return nativeVGGXSmithCorrelatedBinding("default");
}
