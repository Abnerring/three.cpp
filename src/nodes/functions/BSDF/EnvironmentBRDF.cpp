#include "nodes/functions/BSDF/EnvironmentBRDF.h"

#include "nodes/functions/BSDF/DFGLUT.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue body(const JSValue&, const Args& args, const Args&) {
    const auto inputs = arg(args, 0);
    auto dfgInputs = JSValue::object();
    dfgInputs.set("dotNV", inputs.get("dotNV"));
    dfgInputs.set("roughness", inputs.get("roughness"));
    const auto fab = nativeDFGLUT().call({}, {dfgInputs});
    const auto specularColor = inputs.get("specularColor");
    const auto specularF90 = inputs.get("specularF90");
    const auto left = invoke(specularColor, "mul")({fab.get("x")});
    const auto right = invoke(specularF90, "mul")({fab.get("y")});
    return invoke(left, "add")({right});
}
} // namespace

JSValue nativeEnvironmentBRDFModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "EnvironmentBRDF module");
    const auto dfg = nativeDFGLUTModule();
    (void)dfg;
    auto callback = JSValue::function(body, {}, "", 1);
    callback.setFunctionSourceText("( inputs ) => {\r\n\r\n\tconst { dotNV, specularColor, specularF90, roughness } = inputs;\r\n\r\n\tconst fab = DFGLUT( { dotNV, roughness } );\r\n\treturn specularColor.mul( fab.x ).add( specularF90.mul( fab.y ) );\r\n\r\n}");
    auto layout = JSValue::object();
    own(layout, "dotNV", "float");
    own(layout, "specularColor", "vec3");
    own(layout, "specularF90", "float");
    own(layout, "roughness", "float");
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

JSValue nativeEnvironmentBRDFBinding(const std::string& name) {
    const auto keeper = nativeEnvironmentBRDFModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeEnvironmentBRDF() {
    return nativeEnvironmentBRDFBinding("default");
}
