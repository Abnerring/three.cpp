#include "nodes/utils/Timer.h"

#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}

JSValue frameProperty(const JSValue&, const Args& args, const Args& captures) {
    return arg(args, 0).get(captures[0]);
}

JSValue makeTimerUniform(const char* property, const JSValue& nodeType = {}) {
    auto uniform = nodeType.isUndefined()
        ? nativeUniformFactory().call({}, {0})
        : nativeUniformFactory().call({}, {0, nodeType});
    uniform = invoke(uniform, "setGroup")({nativeRenderGroup()});
    auto callback = JSValue::function(frameProperty, {property}, "", 1);
    callback.setFunctionSourceText(std::string("( frame ) => frame.") + property);
    return invoke(uniform, "onRenderUpdate")({callback});
}
} // namespace

JSValue nativeTimerModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "Timer module");
    nativeUniformGroupNodeModule();
    nativeUniformNodeConstructor();

    auto bindings = JSValue::object(nullptr);
    own(bindings, "time", makeTimerUniform("time"));
    own(bindings, "deltaTime", makeTimerUniform("deltaTime"));
    own(bindings, "frameId", makeTimerUniform("frameId", "uint"));

    auto module = JSValue::object(nullptr);
    for (const auto& name : bindings.ownPropertyNames()) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeTimerBinding(const std::string& name) {
    const auto keeper = nativeTimerModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeTimerTime() { return nativeTimerBinding("time"); }
JSValue nativeTimerDeltaTime() { return nativeTimerBinding("deltaTime"); }
JSValue nativeTimerFrameId() { return nativeTimerBinding("frameId"); }
