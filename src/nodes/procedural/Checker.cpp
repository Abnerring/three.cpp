#include "nodes/procedural/Checker.h"

#include "nodes/accessors/UV.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue body(const JSValue&, const Args& args, const Args& captures) {
    const auto table = captures[0];
    auto coord = arg(args, 0);
    if (coord.isUndefined()) coord = table.get("uv").call();

    const auto scaled = invoke(coord, "mul")({2.0});
    const auto cx = invoke(scaled.get("x"), "floor")();
    const auto cy = invoke(scaled.get("y"), "floor")();
    const auto result = invoke(invoke(cx, "add")({cy}), "mod")({2.0});
    return invoke(result, "sign")();
}

} // namespace

JSValue nativeCheckerModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "Checker module");
    nativeUVModule();
    nativeTSLBase();

    auto table = JSValue::object(nullptr);
    own(table, "uv", nativeUVBinding("uv"));

    auto callback = JSValue::function(body, {table}, "", 1);
    callback.setFunctionSourceText(
        "( [ coord = uv() ] ) => {\r\n\r\n"
        "\tconst uv = coord.mul( 2.0 );\r\n\r\n"
        "\tconst cx = uv.x.floor();\r\n"
        "\tconst cy = uv.y.floor();\r\n"
        "\tconst result = cx.add( cy ).mod( 2.0 );\r\n\r\n"
        "\treturn result.sign();\r\n\r\n"
        "}"
    );
    const auto value = nativeTSLBaseBinding("Fn").call({}, {callback});

    auto bindings = JSValue::object(nullptr);
    own(bindings, "checker", value);
    auto module = JSValue::object(nullptr);
    own(module, "checker", value);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeCheckerBinding(const std::string& name) {
    const auto keeper = nativeCheckerModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeChecker() { return nativeCheckerBinding("checker"); }
