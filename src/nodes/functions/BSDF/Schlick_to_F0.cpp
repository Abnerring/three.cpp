#include "nodes/functions/BSDF/Schlick_to_F0.h"

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
    const auto f = inputs.get("f");
    const auto f90 = inputs.get("f90");
    const auto dotVH = inputs.get("dotVH");

    const auto x = invoke(invoke(dotVH, "oneMinus")(), "saturate")();
    const auto x2 = invoke(x, "mul")({x});
    const auto x5 = invoke(x, "mul")({x2, x2});
    const auto clamped = invoke(x5, "clamp")({0, .9999});
    const auto f90Vector = nativeTSLBaseBinding("vec3").call({}, {f90});
    const auto numerator = invoke(f, "sub")({invoke(f90Vector, "mul")({clamped})});
    return invoke(numerator, "div")({invoke(clamped, "oneMinus")()});
}
} // namespace

JSValue nativeSchlickToF0Module() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "Schlick_to_F0 module");
    auto callback = JSValue::function(body, {}, "", 1);
    callback.setFunctionSourceText("( { f, f90, dotVH } ) => {\r\n\r\n\tconst x = dotVH.oneMinus().saturate();\r\n\tconst x2 = x.mul( x );\r\n\tconst x5 = x.mul( x2, x2 ).clamp( 0, .9999 );\r\n\r\n\treturn f.sub( vec3( f90 ).mul( x5 ) ).div( x5.oneMinus() );\r\n\r\n}");
    auto layout = JSValue::object();
    own(layout, "f", "vec3");
    own(layout, "f90", "float");
    own(layout, "dotVH", "float");
    own(layout, "return", "vec3");
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

JSValue nativeSchlickToF0Binding(const std::string& name) {
    const auto keeper = nativeSchlickToF0Module();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeSchlickToF0() {
    return nativeSchlickToF0Binding("default");
}
