#include "nodes/functions/BSDF/D_GGX_Anisotropic.h"

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
    const auto alphaT = inputs.get("alphaT");
    const auto alphaB = inputs.get("alphaB");
    const auto dotNH = inputs.get("dotNH");
    const auto dotTH = inputs.get("dotTH");
    const auto dotBH = inputs.get("dotBH");
    const auto a2 = invoke(alphaT, "mul")({alphaB});
    const auto vec3 = captures[1];
    const auto v = vec3.call({}, {
        invoke(alphaB, "mul")({dotTH}),
        invoke(alphaT, "mul")({dotBH}),
        invoke(a2, "mul")({dotNH})
    });
    const auto v2 = invoke(v, "dot")({v});
    const auto w2 = invoke(a2, "div")({v2});
    const auto value = invoke(a2, "mul")({invoke(w2, "pow2")()});
    return invoke(captures[0], "mul")({value});
}
} // namespace

JSValue nativeDGGXAnisotropicModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "D_GGX_Anisotropic module");
    const auto reciprocalPi = nativeTSLBaseBinding("float").call({}, {1.0 / 3.141592653589793238462643383279502884});
    const auto vec3 = nativeTSLBaseBinding("vec3");
    auto callback = JSValue::function(body, {reciprocalPi, vec3}, "", 1);
    callback.setFunctionSourceText("( { alphaT, alphaB, dotNH, dotTH, dotBH } ) => {\r\n\r\n\tconst a2 = alphaT.mul( alphaB );\r\n\tconst v = vec3( alphaB.mul( dotTH ), alphaT.mul( dotBH ), a2.mul( dotNH ) );\r\n\tconst v2 = v.dot( v );\r\n\tconst w2 = a2.div( v2 );\r\n\r\n\treturn RECIPROCAL_PI.mul( a2.mul( w2.pow2() ) );\r\n\r\n}");
    auto layout = JSValue::object();
    own(layout, "alphaT", "float");
    own(layout, "alphaB", "float");
    own(layout, "dotNH", "float");
    own(layout, "dotTH", "float");
    own(layout, "dotBH", "float");
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

JSValue nativeDGGXAnisotropicBinding(const std::string& name) {
    const auto keeper = nativeDGGXAnisotropicModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeDGGXAnisotropic() {
    return nativeDGGXAnisotropicBinding("default");
}
