// Direct CPU protocol port of three.js WebGLUniformsGroups.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See THREEJS-LICENSE.
#include "renderers/webgl/WebGLUniformsGroups.h"
#include "core/NativeArray.h"
#include "utils.h"
#include <cmath>
#include <unordered_set>

namespace {
#include "WebGLUniformsGroupsSource.inc"
using Args = std::vector<JSValue>;
JSValue argument(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue(); }
void validate(std::initializer_list<const JSValue*> values) {
    for (const auto* value : values) static_cast<void>(JSWeakValue(*value));
}
void literal(JSValue object, const char* name, JSValue value) {
    JSPropertyDescriptor d; d.value = std::move(value);
    d.writable = d.enumerable = d.configurable = true; object.defineProperty(name, d);
}
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(), b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String)
        return a.toString() + b.toString();
    const auto x = a.toNumber(); return x + b.toNumber();
}
double subtract(const JSValue& left, const JSValue& right) {
    const auto x = left.toNumber(); return x - right.toNumber();
}
double remainder(const JSValue& left, const JSValue& right) {
    const auto x = left.toNumber(); return std::fmod(x, right.toNumber());
}
bool array(const JSValue& value) { return value.type() == JSValue::Type::Array; }
bool view(const JSValue& value) { return value.isHostObject("three.AnimationArray"); }
bool scalar(const JSValue& value) {
    return value.type() == JSValue::Type::Number || value.type() == JSValue::Type::Boolean;
}
JSValue single(const JSValue& value) {
    auto result = JSValue::array(1); literal(result, "0", value); return result;
}
struct UniformSize { JSValue boundary = 0, storage = 0; };
UniformSize getUniformSize(const JSValue& value, bool isArrayElement) {
    UniformSize info;
    if (scalar(value)) { info.boundary = 4; info.storage = 4; }
    else if (value.get("isVector2").toBoolean()) { info.boundary = 8; info.storage = 8; }
    else if (value.get("isVector3").toBoolean() || value.get("isColor").toBoolean()) {
        info.boundary = 16; info.storage = 12;
    } else if (value.get("isVector4").toBoolean()) { info.boundary = 16; info.storage = 16; }
    else if (value.get("isMatrix3").toBoolean()) { info.boundary = 16; info.storage = 48; }
    else if (value.get("isMatrix4").toBoolean()) { info.boundary = 16; info.storage = 64; }
    else if (value.get("isTexture").toBoolean())
        ThreeUtils::warn({"WebGLRenderer: Texture samplers can not be part of an uniforms group."});
    else if (view(value)) { info.boundary = 16; info.storage = value.get("byteLength"); }
    else ThreeUtils::warn({"WebGLRenderer: Unsupported uniform value type.", value});
    if (isArrayElement) { info.boundary = 16; info.storage = std::ceil(info.storage.toNumber() / 16) * 16; }
    return info;
}
void writeUniformValue(const JSValue& value, JSValue data, double offset) {
    if (scalar(value)) data.set(JSValue(offset), value);
    else if (value.get("isMatrix3").toBoolean()) {
        // Keep each elements read live; accessor side effects are observable.
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                const auto element = value.get("elements").get(JSValue(row * 3 + column));
                data.set(JSValue(offset + row * 4 + column), element);
            }
            data.set(JSValue(offset + row * 4 + 3), 0);
        }
    } else if (view(value)) {
        const auto set = data.get("set");
        const auto subarray = value.get("subarray");
        const auto count = subtract(data.get("length"), offset);
        const auto source = subarray.call(value, {0, count});
        set.call(data, {source, offset});
    } else {
        const auto toArray = value.get("toArray"); toArray.call(value, {data, offset});
    }
}
bool hasValueChanged(const JSValue& value, JSValue cache, const JSValue& key) {
    if (cache.get(key).isUndefined()) {
        if (scalar(value)) cache.set(key, value);
        else if (view(value)) {
            const auto slice = value.get("slice"); cache.set(key, slice.call(value));
        } else {
            const auto clone = value.get("clone"); cache.set(key, clone.call(value));
        }
        return true;
    }
    const auto cachedObject = cache.get(key);
    if (scalar(value)) {
        if (!cachedObject.strictEquals(value)) { cache.set(key, value); return true; }
    } else if (view(value)) return true;
    else {
        const auto equals = cachedObject.get("equals");
        if (equals.call(cachedObject, {value}).strictEquals(false)) {
            const auto copy = cachedObject.get("copy"); copy.call(cachedObject, {value}); return true;
        }
    }
    return false;
}
bool hasUniformChanged(const JSValue& uniform, double index, double indexArray, JSValue cache) {
    const auto value = uniform.get("value");
    const auto indexString = JSValue(index).toString() + "_" + JSValue(indexArray).toString();
    if (array(value)) {
        if (cache.get(indexString).isUndefined()) cache.set(indexString, JSValue::array());
        const auto cachedArray = cache.get(indexString); bool changed = false;
        for (double k = 0; k < value.get("length").toNumber(); ++k)
            if (hasValueChanged(value.get(JSValue(k)), cachedArray, k)) changed = true;
        return changed;
    }
    return hasValueChanged(value, cache, indexString);
}
void updateUniform(const JSValue& lexical, const JSValue& uniform, double index,
                   double indexArray, JSValue cache) {
    if (!hasUniformChanged(uniform, index, indexArray, cache)) return;
    const auto offset = uniform.get("__offset"), value = uniform.get("value");
    if (array(value)) {
        double arrayOffset = 0;
        for (double k = 0; k < value.get("length").toNumber(); ++k) {
            const auto val = value.get(JSValue(k)); const auto info = getUniformSize(val, true);
            writeUniformValue(val, uniform.get("__data"), arrayOffset);
            arrayOffset += info.storage.toNumber() / NativeArray_detail::constructor(6).get("BYTES_PER_ELEMENT").toNumber();
        }
    } else writeUniformValue(value, uniform.get("__data"), 0);
    const auto gl = lexical.get("gl"), upload = gl.get("bufferSubData");
    const auto target = gl.get("UNIFORM_BUFFER"), data = uniform.get("__data");
    upload.call(gl, {target, offset, data});
}
void updateBufferData(const JSValue& lexical, const JSValue& group) {
    const auto buffer = lexical.get("buffers").get(group.get("id"));
    const auto uniforms = group.get("uniforms"), cache = group.get("__cache"), gl = lexical.get("gl");
    const auto bind = gl.get("bindBuffer"); bind.call(gl, {gl.get("UNIFORM_BUFFER"), buffer});
    const auto count = uniforms.get("length");
    for (double i = 0; i < count.toNumber(); ++i) {
        const auto item = uniforms.get(JSValue(i));
        if (array(item)) {
            const auto length = item.get("length");
            for (double j = 0; j < length.toNumber(); ++j) updateUniform(lexical, item.get(JSValue(j)), i, j, cache);
        } else updateUniform(lexical, item, i, 0, cache);
    }
    const auto unbind = gl.get("bindBuffer"); unbind.call(gl, {gl.get("UNIFORM_BUFFER"), nullptr});
}
void prepareUniformsGroup(const JSValue& group) {
    const auto uniforms = group.get("uniforms"); JSValue offset = 0;
    const auto count = uniforms.get("length");
    for (double i = 0; i < count.toNumber(); ++i) {
        const auto uniformArray = array(uniforms.get(JSValue(i))) ? uniforms.get(JSValue(i)) : single(uniforms.get(JSValue(i)));
        const auto length = uniformArray.get("length");
        for (double j = 0; j < length.toNumber(); ++j) {
            auto uniform = uniformArray.get(JSValue(j));
            const auto values = array(uniform.get("value")) ? uniform.get("value") : single(uniform.get("value"));
            const bool isArray = array(uniforms.get(JSValue(i))) || array(uniform.get("value"));
            JSValue start = offset; const auto size = values.get("length");
            for (double k = 0; k < size.toNumber(); ++k) {
                const auto value = values.get(JSValue(k)); const auto info = getUniformSize(value, isArray);
                const auto chunkOffset = remainder(offset, 16), chunkPadding = remainder(chunkOffset, info.boundary);
                const auto chunkStart = chunkOffset + chunkPadding;
                offset = add(offset, chunkPadding);
                if (chunkStart != 0 && (16 - chunkStart) < info.storage.toNumber()) offset = add(offset, 16 - chunkStart);
                if (k == 0) start = offset;
                offset = add(offset, info.storage);
            }
            const auto kind = uniform.get("type").strictEquals("int") ? 4 : uniform.get("type").strictEquals("uint") ? 5 : 6;
            const auto bytes = subtract(offset, start);
            const auto elements = bytes / NativeArray_detail::constructor(6).get("BYTES_PER_ELEMENT").toNumber();
            uniform.set("__data", NativeArray_detail::constructor(kind).construct({elements}));
            uniform.set("__offset", start);
        }
    }
    const auto chunkOffset = remainder(offset, 16);
    if (chunkOffset > 0) offset = add(offset, 16 - chunkOffset);
    auto writable = group; writable.set("__size", offset); writable.set("__cache", JSValue::object());
}
JSValue createBuffer(const JSValue& lexical, const JSValue& group) {
    const auto gl = lexical.get("gl"), create = gl.get("createBuffer"), buffer = create.call(gl);
    const auto size = group.get("__size"), usage = group.get("usage");
    const auto bind = gl.get("bindBuffer"); bind.call(gl, {gl.get("UNIFORM_BUFFER"), buffer});
    const auto allocate = gl.get("bufferData"); allocate.call(gl, {gl.get("UNIFORM_BUFFER"), size, usage});
    const auto unbind = gl.get("bindBuffer"); unbind.call(gl, {gl.get("UNIFORM_BUFFER"), nullptr});
    return buffer;
}
JSValue bind(const JSValue&, const Args& args, const Args& captures) {
    const auto lexical = captures[0], group = argument(args, 0), program = argument(args, 1).get("program");
    const auto state = lexical.get("state"), fn = state.get("uniformBlockBinding");
    const auto buffer = lexical.get("buffers").get(group.get("id")); fn.call(state, {group, program, buffer}); return {};
}
JSValue update(const JSValue&, const Args& args, const Args& captures) {
    const auto lexical = captures[0], group = argument(args, 0);
    auto buffer = lexical.get("buffers").get(group.get("id"));
    if (buffer.isUndefined()) {
        prepareUniformsGroup(group); buffer = createBuffer(lexical, group);
        lexical.get("buffers").set(group.get("id"), buffer);
        const auto listen = group.get("addEventListener"); listen.call(group, {"dispose", lexical.get("onDispose")});
    }
    const auto program = argument(args, 1).get("program"), state = lexical.get("state");
    const auto mapping = state.get("updateUBOMapping"); mapping.call(state, {group, program});
    const auto frame = lexical.get("info").get("render").get("frame");
    if (!lexical.get("updateList").get(group.get("id")).strictEquals(frame)) {
        updateBufferData(lexical, group); lexical.get("updateList").set(group.get("id"), frame);
    }
    return {};
}
JSValue onDispose(const JSValue&, const Args& args, const Args& captures) {
    const auto lexical = captures[0], group = argument(args, 0).get("target");
    const auto remove = group.get("removeEventListener"); remove.call(group, {"dispose", lexical.get("onDispose")});
    const auto gl = lexical.get("gl"), erase = gl.get("deleteBuffer");
    const auto buffer = lexical.get("buffers").get(group.get("id")); erase.call(gl, {buffer});
    lexical.get("buffers").erase(group.get("id")); lexical.get("updateList").erase(group.get("id")); return {};
}
JSValue dispose(const JSValue&, const Args&, const Args& captures) {
    auto lexical = captures[0]; const auto gl = lexical.get("gl");
    std::unordered_set<std::string> visited;
    // for-in snapshots each prototype's names; descriptors and lexical buffers
    // remain live during callbacks, and deleted own names can reveal prototypes.
    for (auto current = lexical.get("buffers"); !current.isNull(); current = current.getPrototype()) {
        const auto names = current.ownPropertyNames();
        for (const auto& id : names) {
            if (visited.contains(id)) continue;
            const auto descriptor = current.getOwnPropertyDescriptor(id);
            if (!descriptor) continue;
            visited.insert(id);
            if (!descriptor->enumerable.value_or(false)) continue;
            const auto erase = gl.get("deleteBuffer"), buffer = lexical.get("buffers").get(id); erase.call(gl, {buffer});
        }
    }
    lexical.set("buffers", JSValue::object()); lexical.set("updateList", JSValue::object()); return {};
}
JSValue construct(const JSValue&, const Args& args, const Args&) {
    auto lexical = JSValue::object(nullptr);
    literal(lexical, "gl", argument(args, 0)); literal(lexical, "info", argument(args, 1));
    literal(lexical, "state", argument(args, 3));
    literal(lexical, "buffers", JSValue::object()); literal(lexical, "updateList", JSValue::object());
    literal(lexical, "onDispose", JSValue::constructor(onDispose, {lexical}, "onUniformsGroupsDispose", 1, JSValue::ConstructorKind::Callable));
    auto result = JSValue::object();
    literal(result, "bind", JSValue::constructor(bind, {lexical}, "bind", 2, JSValue::ConstructorKind::Callable));
    literal(result, "update", JSValue::constructor(update, {lexical}, "update", 2, JSValue::ConstructorKind::Callable));
    literal(result, "dispose", JSValue::constructor(dispose, {lexical}, "dispose", 0, JSValue::ConstructorKind::Callable));
    lexical.get("onDispose").setFunctionSourceText(source_onUniformsGroupsDispose);
    result.get("bind").setFunctionSourceText(source_bind);
    result.get("update").setFunctionSourceText(source_update);
    result.get("dispose").setFunctionSourceText(source_dispose);
    return result;
}
} // namespace
JSValue nativeWebGLUniformsGroupsConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isCallable()) return *cached;
    auto result = JSValue::constructor(construct, {}, "WebGLUniformsGroups", 4, JSValue::ConstructorKind::Callable);
    result.setFunctionSourceText(source_WebGLUniformsGroups);
    result.retainForHeapLifetime(); weak = JSWeakValue(result); return result;
}
WebGLUniformsGroups::WebGLUniformsGroups(const JSValue& gl, const JSValue& info,
                                       const JSValue& capabilities, const JSValue& state) {
    validate({&gl, &info, &capabilities, &state}); object_ = nativeWebGLUniformsGroupsConstructor().call({}, {gl, info, capabilities, state});
}
JSValue WebGLUniformsGroups::bind(const JSValue& group, const JSValue& program) {
    validate({&object_, &group, &program}); const auto fn = object_.get("bind"); return fn.call(object_, {group, program});
}
JSValue WebGLUniformsGroups::update(const JSValue& group, const JSValue& program) {
    validate({&object_, &group, &program}); const auto fn = object_.get("update"); return fn.call(object_, {group, program});
}
JSValue WebGLUniformsGroups::dispose() {
    validate({&object_}); const auto fn = object_.get("dispose"); return fn.call(object_);
}
