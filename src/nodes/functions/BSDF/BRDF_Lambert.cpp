#include "nodes/functions/BSDF/BRDF_Lambert.h"

#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

#include <cmath>

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}

JSValue body(const JSValue&, const Args& args, const Args&) {
    const auto inputs = arg(args, 0);
    return invoke(inputs.get("diffuseColor"), "mul")({1.0 / 3.141592653589793238462643383279502884});
}
} // namespace

JSValue nativeBRDFLambertModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "BRDF_Lambert module");
    auto callback = JSValue::function(body, {}, "", 1);
    callback.setFunctionSourceText("( inputs ) => {\r\n\r\n\treturn inputs.diffuseColor.mul( 1 / Math.PI ); // punctual light\r\n\r\n}");
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

JSValue nativeBRDFLambertBinding(const std::string& name) {
    const auto keeper = nativeBRDFLambertModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeBRDFLambert() {
    return nativeBRDFLambertBinding("default");
}

