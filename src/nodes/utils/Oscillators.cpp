#include "nodes/utils/Oscillators.h"

#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/utils/Timer.h"

#include <numbers>

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue input(const Args& args, const Args& captures) {
    return args.empty() || args[0].isUndefined() || args[0].isNull() ? captures[0] : args[0];
}

JSValue sine(const JSValue&, const Args& args, const Args& captures) {
    auto value = invoke(input(args, captures), "add")({0.75});
    value = invoke(value, "mul")({2.0 * std::numbers::pi});
    value = invoke(value, "sin")();
    value = invoke(value, "mul")({0.5});
    return invoke(value, "add")({0.5});
}

JSValue square(const JSValue&, const Args& args, const Args& captures) {
    return invoke(invoke(input(args, captures), "fract")(), "round")();
}

JSValue triangle(const JSValue&, const Args& args, const Args& captures) {
    auto value = invoke(input(args, captures), "add")({0.5});
    value = invoke(value, "fract")();
    value = invoke(value, "mul")({2});
    value = invoke(value, "sub")({1});
    return invoke(value, "abs")();
}

JSValue sawtooth(const JSValue&, const Args& args, const Args& captures) {
    return invoke(input(args, captures), "fract")();
}
} // namespace

JSValue nativeOscillatorsModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "Oscillators module");
    const auto timer = nativeTimerModule();
    const auto time = timer.get("time");

    auto bindings = JSValue::object(nullptr);
    own(bindings, "oscSine", JSValue::function(sine, {time}, "oscSine", 0));
    own(bindings, "oscSquare", JSValue::function(square, {time}, "oscSquare", 0));
    own(bindings, "oscTriangle", JSValue::function(triangle, {time}, "oscTriangle", 0));
    own(bindings, "oscSawtooth", JSValue::function(sawtooth, {time}, "oscSawtooth", 0));

    auto module = JSValue::object(nullptr);
    for (const auto& name : bindings.ownPropertyNames()) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeOscillatorsBinding(const std::string& name) {
    const auto keeper = nativeOscillatorsModule();
    (void)keeper;
    return lexical().lock()->get(name);
}
