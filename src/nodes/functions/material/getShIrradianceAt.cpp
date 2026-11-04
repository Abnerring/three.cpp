#include "nodes/functions/material/getShIrradianceAt.h"

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
    const auto normal = inputs.get(JSValue(0));
    const auto shCoefficients = inputs.get(JSValue(1));
    const auto x = normal.get("x");
    const auto y = normal.get("y");
    const auto z = normal.get("z");
    const auto element = invoke(shCoefficients, "element");

    auto result = invoke(element({0}), "mul")({0.886227});
    auto addTerm = [&](int index, const JSValue& scale, const JSValue& component) {
        result = invoke(result, "add")({invoke(invoke(element({index}), "mul")({scale}), "mul")({component})});
    };
    addTerm(1, 2.0 * 0.511664, y);
    addTerm(2, 2.0 * 0.511664, z);
    addTerm(3, 2.0 * 0.511664, x);

    result = invoke(result, "add")({invoke(invoke(invoke(element({4}), "mul")({2.0 * 0.429043}), "mul")({x}), "mul")({y})});
    result = invoke(result, "add")({invoke(invoke(invoke(element({5}), "mul")({2.0 * 0.429043}), "mul")({y}), "mul")({z})});
    const auto zTerm = invoke(invoke(invoke(z, "mul")({z}), "mul")({0.743125}), "sub")({0.247708});
    result = invoke(result, "add")({invoke(element({6}), "mul")({zTerm})});
    result = invoke(result, "add")({invoke(invoke(invoke(element({7}), "mul")({2.0 * 0.429043}), "mul")({x}), "mul")({z})});
    const auto xx = invoke(x, "mul")({x});
    const auto yy = invoke(y, "mul")({y});
    result = invoke(result, "add")({invoke(element({8}), "mul")({invoke(invoke(xx, "sub")({yy}), "mul")({0.429043})})});
    return result;
}
} // namespace

JSValue nativeGetShIrradianceAtModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "getShIrradianceAt module");
    auto callback = JSValue::function(body, {}, "", 1);
    callback.setFunctionSourceText("( [ normal, shCoefficients ] ) => {\r\n\r\n\t// normal is assumed to have unit length\r\n\r\n\tconst x = normal.x, y = normal.y, z = normal.z;\r\n\r\n\t// band 0\r\n\tlet result = shCoefficients.element( 0 ).mul( 0.886227 );\r\n\r\n\t// band 1\r\n\tresult = result.add( shCoefficients.element( 1 ).mul( 2.0 * 0.511664 ).mul( y ) );\r\n\tresult = result.add( shCoefficients.element( 2 ).mul( 2.0 * 0.511664 ).mul( z ) );\r\n\tresult = result.add( shCoefficients.element( 3 ).mul( 2.0 * 0.511664 ).mul( x ) );\r\n\r\n\t// band 2\r\n\tresult = result.add( shCoefficients.element( 4 ).mul( 2.0 * 0.429043 ).mul( x ).mul( y ) );\r\n\tresult = result.add( shCoefficients.element( 5 ).mul( 2.0 * 0.429043 ).mul( y ).mul( z ) );\r\n\tresult = result.add( shCoefficients.element( 6 ).mul( z.mul( z ).mul( 0.743125 ).sub( 0.247708 ) ) );\r\n\tresult = result.add( shCoefficients.element( 7 ).mul( 2.0 * 0.429043 ).mul( x ).mul( z ) );\r\n\tresult = result.add( shCoefficients.element( 8 ).mul( 0.429043 ).mul( mul( x, x ).sub( mul( y, y ) ) ) );\r\n\r\n\treturn result;\r\n\r\n}");
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

JSValue nativeGetShIrradianceAtBinding(const std::string& name) {
    const auto keeper = nativeGetShIrradianceAtModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeGetShIrradianceAt() {
    return nativeGetShIrradianceAtBinding("default");
}

