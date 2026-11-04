#include "nodes/functions/BSDF/F_Schlick.h"

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
    const auto f0 = inputs.get("f0");
    const auto f90 = inputs.get("f90");
    const auto dotVH = inputs.get("dotVH");
    const auto scaled = invoke(dotVH, "mul")({-5.55473});
    const auto shifted = invoke(scaled, "sub")({6.98316});
    const auto fresnel = invoke(invoke(shifted, "mul")({dotVH}), "exp2")();
    const auto left = invoke(f0, "mul")({invoke(fresnel, "oneMinus")()});
    const auto right = invoke(f90, "mul")({fresnel});
    return invoke(left, "add")({right});
}
} // namespace

JSValue nativeFSchlickModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "F_Schlick module");
    auto callback = JSValue::function(body, {}, "", 1);
    callback.setFunctionSourceText("( { f0, f90, dotVH } ) => {\r\n\r\n\t// Original approximation by Christophe Schlick '94\r\n\t// float fresnel = pow( 1.0 - dotVH, 5.0 );\r\n\r\n\t// Optimized variant (presented by Epic at SIGGRAPH '13)\r\n\t// https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf\r\n\tconst fresnel = dotVH.mul( - 5.55473 ).sub( 6.98316 ).mul( dotVH ).exp2();\r\n\r\n\treturn f0.mul( fresnel.oneMinus() ).add( f90.mul( fresnel ) );\r\n\r\n}");
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

JSValue nativeFSchlickBinding(const std::string& name) {
    const auto keeper = nativeFSchlickModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeFSchlick() {
    return nativeFSchlickBinding("default");
}
