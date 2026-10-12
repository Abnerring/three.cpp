#include "renderers/webgl/WebGLBufferRenderer.h"

namespace {
using Args = std::vector<JSValue>;
JSValue argument(const Args& args, std::size_t index) { return index < args.size() ? args[index] : JSValue(); }
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive();
    const auto b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String)
        return a.toString() + b.toString();
    const auto x = a.toNumber();
    const auto y = b.toNumber();
    return x + y;
}
JSValue method(const JSValue&, const Args& args, const Args& captures) {
    const auto name = captures[0].string();
    auto state = captures[1];
    if (name == "setMode") { state.set("mode", argument(args, 0)); return {}; }
    const auto gl = captures[2], extensions = captures[3], info = captures[4];
    const auto start = argument(args, 0), count = argument(args, 1);
    if (name == "render") {
        const auto draw = gl.get("drawArrays");
        const auto mode = state.get("mode");
        draw.call(gl, {mode, start, count});
        const auto update = info.get("update");
        const auto updatedMode = state.get("mode");
        update.call(info, {count, updatedMode, 1});
        return {};
    }
    const auto instances = argument(args, 2);
    if (instances.strictEquals(JSValue(0))) return {};
    if (name == "renderInstances") {
        const auto draw = gl.get("drawArraysInstanced");
        const auto mode = state.get("mode");
        draw.call(gl, {mode, start, count, instances});
        const auto update = info.get("update");
        const auto updatedMode = state.get("mode");
        update.call(info, {count, updatedMode, instances});
        return {};
    }
    const auto getExtension = extensions.get("get");
    const auto extension = getExtension.call(extensions, {"WEBGL_multi_draw"});
    const auto draw = extension.get("multiDrawArraysWEBGL");
    const auto mode = state.get("mode");
    draw.call(extension, {mode, start, 0, count, 0, instances});
    JSValue elementCount = 0;
    for (double i = 0; i < instances.toNumber(); ++i) {
        const auto previous = elementCount;
        const auto value = count.get(JSValue(i).toString());
        elementCount = add(previous, value);
    }
    const auto update = info.get("update");
    const auto updatedMode = state.get("mode");
    update.call(info, {elementCount, updatedMode, 1});
    return {};
}
}
void WebGLBufferRenderer::initialize(JSValue self, JSValue gl, JSValue extensions, JSValue info) {
    auto state = JSValue::object(nullptr); state.set("mode", JSValue());
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
        {"setMode",1}, {"render",2}, {"renderInstances",3}, {"renderMultiDraw",3}})
        self.set(name, JSValue::constructor(method, {name, state, gl, extensions, info}, name, arity, JSValue::ConstructorKind::Callable));
}
JSValue WebGLBufferRenderer::prototype() {
    static thread_local auto value = JSValue::constructor([](const JSValue& self, const Args& args, const Args&)->JSValue {
        initialize(self, argument(args,0), argument(args,1), argument(args,2)); return {};
    }, {}, "WebGLBufferRenderer", 3, JSValue::ConstructorKind::Callable).get("prototype");
    return value;
}
WebGLBufferRenderer::WebGLBufferRenderer(JSValue gl, JSValue extensions, JSValue info) : object_(JSValue::object(prototype())) {
    initialize(object_, std::move(gl), std::move(extensions), std::move(info));
}
JSValue WebGLBufferRenderer::setMode(JSValue value) { return object_.get("setMode").call(object_, {value}); }
JSValue WebGLBufferRenderer::render(JSValue start, JSValue count) { return object_.get("render").call(object_, {start,count}); }
JSValue WebGLBufferRenderer::renderInstances(JSValue start, JSValue count, JSValue instances) {
    return object_.get("renderInstances").call(object_, {start,count,instances});
}
JSValue WebGLBufferRenderer::renderMultiDraw(JSValue starts, JSValue counts, JSValue drawCount) {
    return object_.get("renderMultiDraw").call(object_, {starts,counts,drawCount});
}
