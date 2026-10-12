#include "renderers/webgl/WebGLIndexedBufferRenderer.h"

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
double byteOffset(const JSValue& start, const JSValue& elementSize) {
    const auto x = start.toNumber();
    const auto y = elementSize.toNumber();
    return x * y;
}
JSValue method(const JSValue&, const Args& args, const Args& captures) {
    const auto name = captures[0].string();
    auto state = captures[1];
    if (name == "setMode") { state.set("mode", argument(args,0)); return {}; }
    if (name == "setIndex") {
        const auto value = argument(args,0);
        state.set("type", value.get("type"));
        state.set("bytesPerElement", value.get("bytesPerElement"));
        return {};
    }
    const auto gl = captures[2], extensions = captures[3], info = captures[4];
    const auto start = argument(args,0), count = argument(args,1);
    if (name == "render") {
        const auto draw = gl.get("drawElements");
        const auto mode = state.get("mode"), type = state.get("type"), size = state.get("bytesPerElement");
        const auto offset = byteOffset(start, size);
        draw.call(gl, {mode,count,type,offset});
        const auto update = info.get("update");
        const auto updatedMode = state.get("mode");
        update.call(info, {count,updatedMode,1});
        return {};
    }
    const auto instances = argument(args,2);
    if (instances.strictEquals(JSValue(0))) return {};
    if (name == "renderInstances") {
        const auto draw = gl.get("drawElementsInstanced");
        const auto mode = state.get("mode"), type = state.get("type"), size = state.get("bytesPerElement");
        const auto offset = byteOffset(start, size);
        draw.call(gl, {mode,count,type,offset,instances});
        const auto update = info.get("update");
        const auto updatedMode = state.get("mode");
        update.call(info, {count,updatedMode,instances});
        return {};
    }
    const auto getExtension = extensions.get("get");
    const auto extension = getExtension.call(extensions, {"WEBGL_multi_draw"});
    const auto draw = extension.get("multiDrawElementsWEBGL");
    const auto mode = state.get("mode"), type = state.get("type");
    draw.call(extension, {mode,count,0,type,start,0,instances});
    JSValue elementCount = 0;
    for (double i = 0; i < instances.toNumber(); ++i) {
        const auto previous = elementCount;
        const auto value = count.get(JSValue(i).toString());
        elementCount = add(previous, value);
    }
    const auto update = info.get("update");
    const auto updatedMode = state.get("mode");
    update.call(info, {elementCount,updatedMode,1});
    return {};
}
}
void WebGLIndexedBufferRenderer::initialize(JSValue self, JSValue gl, JSValue extensions, JSValue info) {
    auto state = JSValue::object(nullptr); state.set("mode",JSValue()); state.set("type",JSValue()); state.set("bytesPerElement",JSValue());
    for (const auto& [name,arity] : std::initializer_list<std::pair<const char*,unsigned>>{
        {"setMode",1}, {"setIndex",1}, {"render",2}, {"renderInstances",3}, {"renderMultiDraw",3}})
        self.set(name,JSValue::constructor(method,{name,state,gl,extensions,info},name,arity,JSValue::ConstructorKind::Callable));
}
JSValue WebGLIndexedBufferRenderer::prototype() {
    static thread_local auto value = JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue {
        initialize(self,argument(args,0),argument(args,1),argument(args,2));return {};
    },{} ,"WebGLIndexedBufferRenderer",3,JSValue::ConstructorKind::Callable).get("prototype");
    return value;
}
WebGLIndexedBufferRenderer::WebGLIndexedBufferRenderer(JSValue gl,JSValue extensions,JSValue info) : object_(JSValue::object(prototype())) {
    initialize(object_,std::move(gl),std::move(extensions),std::move(info));
}
JSValue WebGLIndexedBufferRenderer::setMode(JSValue value) {return object_.get("setMode").call(object_,{value});}
JSValue WebGLIndexedBufferRenderer::setIndex(JSValue value) {return object_.get("setIndex").call(object_,{value});}
JSValue WebGLIndexedBufferRenderer::render(JSValue start,JSValue count) {return object_.get("render").call(object_,{start,count});}
JSValue WebGLIndexedBufferRenderer::renderInstances(JSValue start,JSValue count,JSValue instances) {
    return object_.get("renderInstances").call(object_,{start,count,instances});
}
JSValue WebGLIndexedBufferRenderer::renderMultiDraw(JSValue starts,JSValue counts,JSValue drawCount) {
    return object_.get("renderMultiDraw").call(object_,{starts,counts,drawCount});
}
