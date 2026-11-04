#include "nodes/functions/BSDF/D_GGX.h"

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
    const auto dotNH = inputs.get("dotNH");

    const auto a2 = invoke(alpha, "pow2")();
    const auto dotNH2 = invoke(dotNH, "pow2")();
    const auto denom = invoke(invoke(dotNH2, "mul")({invoke(a2, "oneMinus")()}), "oneMinus")();
    return invoke(invoke(a2, "div")({invoke(denom, "pow2")()}), "mul")({1.0 / 3.141592653589793238462643383279502884});
}
} // namespace

JSValue nativeDGGXModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "D_GGX module");
    auto callback = JSValue::function(body, {}, "", 1);
    callback.setFunctionSourceText("( { alpha, dotNH } ) => {\r\n\r\n\tconst a2 = alpha.pow2();\r\n\r\n\tconst denom = dotNH.pow2().mul( a2.oneMinus() ).oneMinus(); // avoid alpha = 0 with dotNH = 1\r\n\r\n\treturn a2.div( denom.pow2() ).mul( 1 / Math.PI );\r\n\r\n}");
    auto layout = JSValue::object();
    own(layout, "alpha", "float");
    own(layout, "dotNH", "float");
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

JSValue nativeDGGXBinding(const std::string& name) {
    const auto keeper = nativeDGGXModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeDGGX() {
    return nativeDGGXBinding("default");
}
