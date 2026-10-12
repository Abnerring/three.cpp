#include "renderers/webgl/WebGLInfo.h"
#include <iostream>

namespace {
using Args = std::vector<JSValue>;
thread_local JSValue consoleFunction = nullptr;
JSValue argument(const Args& args, std::size_t index) {
    return index < args.size() ? args[index] : JSValue();
}
void literalProperty(JSValue object, const char* key, JSValue value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = std::move(value);
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}
JSValue addNumber(const JSValue& left, double right) {
    const auto primitive = left.toPrimitive();
    if (primitive.type() == JSValue::Type::String)
        return primitive.toString() + JSValue(right).toString();
    const auto number = primitive.toNumber();
    return number + right;
}
void unknownMode(const JSValue& mode) {
    const JSValue message = "THREE.WebGLInfo: Unknown draw mode:";
    const auto callback = consoleFunction;
    if (callback.toBoolean()) {
        callback.call(JSValue(), {"error", message, mode});
        return;
    }
    if (mode.toBoolean() && mode.get("isStackTrace").toBoolean()) {
        const auto getError = mode.get("getError");
        const auto error = getError.call(mode, {message});
        std::cerr << error.trace().dump() << '\n';
    } else {
        // trace inspects the managed value without coercing object properties.
        std::cerr << message.string() << ' ' << mode.trace().dump() << '\n';
    }
}
JSValue update(const JSValue&, const Args& args, const Args& captures) {
    auto render = captures[0];
    const auto gl = captures[1];
    const auto calls = render.get("calls").toNumber();
    render.set("calls", calls + 1);
    const auto count = argument(args, 0);
    const auto mode = argument(args, 1);
    const auto instanceCount = argument(args, 2);
    const char* field = nullptr;
    enum class Operation { Triangles, Lines, Strip, Loop, Points } operation;
    if (mode.strictEquals(gl.get("TRIANGLES"))) { field = "triangles"; operation = Operation::Triangles; }
    else if (mode.strictEquals(gl.get("LINES"))) { field = "lines"; operation = Operation::Lines; }
    else if (mode.strictEquals(gl.get("LINE_STRIP"))) { field = "lines"; operation = Operation::Strip; }
    else if (mode.strictEquals(gl.get("LINE_LOOP"))) { field = "lines"; operation = Operation::Loop; }
    else if (mode.strictEquals(gl.get("POINTS"))) { field = "points"; operation = Operation::Points; }
    else { unknownMode(mode); return {}; }

    // Compound assignment reads the old field before either operand converts.
    const auto previous = render.get(field);
    double contribution;
    if (operation == Operation::Triangles || operation == Operation::Lines || operation == Operation::Strip) {
        const auto convertedCount = count.toNumber();
        const auto right = operation == Operation::Triangles ? convertedCount / 3
            : operation == Operation::Lines ? convertedCount / 2 : convertedCount - 1;
        const auto instances = instanceCount.toNumber();
        contribution = instances * right;
    } else {
        const auto instances = instanceCount.toNumber();
        const auto convertedCount = count.toNumber();
        contribution = instances * convertedCount;
    }
    render.set(field, addNumber(previous, contribution));
    return {};
}
JSValue reset(const JSValue&, const Args&, const Args& captures) {
    auto render = captures[0];
    render.set("calls", 0);
    render.set("triangles", 0);
    render.set("points", 0);
    render.set("lines", 0);
    return {};
}
}

WebGLInfo::WebGLInfo(JSValue gl) : object_(JSValue::object()) {
    auto memory = JSValue::object();
    literalProperty(memory, "geometries", 0); literalProperty(memory, "textures", 0);
    auto render = JSValue::object();
    literalProperty(render, "frame", 0); literalProperty(render, "calls", 0); literalProperty(render, "triangles", 0);
    literalProperty(render, "points", 0); literalProperty(render, "lines", 0);
    literalProperty(object_, "memory", memory); literalProperty(object_, "render", render);
    literalProperty(object_, "programs", nullptr); literalProperty(object_, "autoReset", true);
    literalProperty(object_, "reset", JSValue::constructor(::reset, {render}, "reset", 0, JSValue::ConstructorKind::Callable));
    literalProperty(object_, "update", JSValue::constructor(::update, {render, gl}, "update", 3, JSValue::ConstructorKind::Callable));
}
JSValue WebGLInfo::update(JSValue count, JSValue mode, JSValue instanceCount) {
    return object_.get("update").call(object_, {count, mode, instanceCount});
}
JSValue WebGLInfo::reset() { return object_.get("reset").call(object_); }
void WebGLInfo::setConsoleFunction(JSValue callback) { consoleFunction = std::move(callback); }
JSValue WebGLInfo::getConsoleFunction() { return consoleFunction; }
