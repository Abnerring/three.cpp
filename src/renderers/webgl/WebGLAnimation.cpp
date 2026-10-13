// Ported from three.js WebGLAnimation.js; see THREEJS-LICENSE.
#include "renderers/webgl/WebGLAnimation.h"

namespace {
using Args = std::vector<JSValue>;
JSValue argument(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue(); }
void literal(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}
void request(JSValue state) {
    const auto context = state.get("context");
    const auto method = context.get("requestAnimationFrame");
    const auto id = method.call(context, {state.get("onAnimationFrame")});
    state.set("requestId", id);
}
JSValue onAnimationFrame(const JSValue&, const Args& args, const Args& captures) {
    const auto state = captures[0];
    request(state);
    const auto loop = state.get("animationLoop");
    loop.call({}, {argument(args, 0), argument(args, 1)});
    return {};
}
JSValue start(const JSValue&, const Args&, const Args& captures) {
    auto state = captures[0];
    if (state.get("isAnimating").strictEquals(true)) return {};
    if (state.get("animationLoop").isNull()) return {};
    if (state.get("context").isNull()) return {};
    request(state);
    state.set("isAnimating", true);
    return {};
}
JSValue stop(const JSValue&, const Args&, const Args& captures) {
    auto state = captures[0];
    if (!state.get("context").isNull()) {
        const auto context = state.get("context");
        const auto method = context.get("cancelAnimationFrame");
        method.call(context, {state.get("requestId")});
    }
    state.set("isAnimating", false);
    return {};
}
JSValue setAnimationLoop(const JSValue&, const Args& args, const Args& captures) {
    auto state = captures[0]; state.set("animationLoop", argument(args, 0)); return {};
}
JSValue setContext(const JSValue&, const Args& args, const Args& captures) {
    auto state = captures[0]; state.set("context", argument(args, 0)); return {};
}
JSValue construct(const JSValue&, const Args&, const Args&) {
    auto state = JSValue::object(nullptr);
    literal(state, "context", nullptr);
    literal(state, "isAnimating", false);
    literal(state, "animationLoop", nullptr);
    literal(state, "requestId", nullptr);
    literal(state, "onAnimationFrame", JSValue::constructor(onAnimationFrame, {state},
        "onAnimationFrame", 2, JSValue::ConstructorKind::Callable));
    auto result = JSValue::object();
    literal(result, "start", JSValue::constructor(start, {state}, "start", 0, JSValue::ConstructorKind::Callable));
    literal(result, "stop", JSValue::constructor(stop, {state}, "stop", 0, JSValue::ConstructorKind::Callable));
    literal(result, "setAnimationLoop", JSValue::constructor(setAnimationLoop, {state},
        "setAnimationLoop", 1, JSValue::ConstructorKind::Callable));
    literal(result, "setContext", JSValue::constructor(setContext, {state},
        "setContext", 1, JSValue::ConstructorKind::Callable));
    return result;
}
JSValue invoke(const JSValue& object, const char* method, const Args& args = {}) {
    return object.get(method).call(object, args);
}
}
JSValue nativeWebGLAnimationConstructor() {
    static thread_local JSWeakValue cached;
    if (const auto value = cached.lock(); value && value->isCallable()) return *value;
    auto value = JSValue::constructor(construct, {}, "WebGLAnimation", 0, JSValue::ConstructorKind::Callable);
    value.retainForHeapLifetime(); cached = JSWeakValue(value); return value;
}
WebGLAnimation::WebGLAnimation() : object_(nativeWebGLAnimationConstructor().call({})) {}
JSValue WebGLAnimation::start() { return invoke(object(), "start"); }
JSValue WebGLAnimation::stop() { return invoke(object(), "stop"); }
JSValue WebGLAnimation::setAnimationLoop(const JSValue& value) {
    static_cast<void>(JSWeakValue(value)); return invoke(object(), "setAnimationLoop", {value});
}
JSValue WebGLAnimation::setContext(const JSValue& value) {
    static_cast<void>(JSWeakValue(value)); return invoke(object(), "setContext", {value});
}
