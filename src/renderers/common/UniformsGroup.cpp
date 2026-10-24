#include "renderers/common/UniformsGroup.h"
#include "renderers/common/UniformBuffer.h"
#include "core/JSIterator.h"
#include "core/JSMap.h"
#include "core/JSSet.h"
#include "core/JSModuleObject.h"
#include "core/NativeArray.h"
#include "utils.h"
#include <cmath>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
struct Invocation {
    JSValue object, function;
    JSValue operator()(const Args& args = {}) const { return function.call(object, args); }
};
Invocation invoke(const JSValue& object, const char* key) { return {object, object.get(key)}; }
JSValue superCall(const JSValue& self, const char* name, const Args& args = {}) {
    return nativeUniformsGroupConstructor().get("prototype").getPrototype().get(name, self).call(self, args);
}
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(), b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String)
        return a.toString() + b.toString();
    const auto x = a.toNumber(), y = b.toNumber();
    return x + y;
}
// A computed property's raw key is converted by PutValue, so the inner
// assignment converts its key and writes before the outer key conversion.
void assignPair(JSValue buffer, JSValue values, const JSValue& offset, const JSValue& value) {
    values.set(offset, value); buffer.set(offset, value);
}
void assignPair(JSValue buffer, JSValue values, const JSValue& offset, double index,
                const JSValue& source, const char* component) {
    const auto bufferKey = add(offset, index).toPropertyKey();
    const auto valuesKey = add(offset, index).toPropertyKey();
    const auto value = source.get(component);
    values.set(valuesKey, value); buffer.set(bufferKey, value);
}
void assignMatrixPair(JSValue buffer, JSValue values, const JSValue& offset, double index,
                      const JSValue& elements, double element) {
    const auto bufferKey = add(offset, index).toPropertyKey();
    const auto valuesKey = add(offset, index).toPropertyKey();
    const auto value = elements.get(JSValue(element));
    values.set(valuesKey, value); buffer.set(bufferKey, value);
}
bool arraysEqual(const JSValue& a, const JSValue& b, const JSValue& offset) {
    const auto length = b.get("length");
    for (double i = 0; i < length.toNumber(); ++i) {
        const auto left = a.get(add(offset, i));
        if (!left.strictEquals(b.get(JSValue(i)))) return false;
    }
    return true;
}
void setArray(JSValue a, const JSValue& b, const JSValue& offset) {
    const auto length = b.get("length");
    for (double i = 0; i < length.toNumber(); ++i) {
        const auto key = add(offset, i).toPropertyKey();
        a.set(key, b.get(JSValue(i)));
    }
}
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string(); const auto uniform = arg(args, 0);
    if (name == "addUniformUpdateRange") {
        const auto index = uniform.get("index");
        if (invoke(self.get("_addedIndices"), "has")({index}).toBoolean()) return {};
        auto range = invoke(self.get("_updateRangeCache"), "get")({index});
        if (range.isUndefined()) {
            range = JSValue::object();
            for (const auto* key : {"start", "count"}) {
                JSPropertyDescriptor d; d.value = 0; d.writable = d.enumerable = d.configurable = true;
                range.defineProperty(key, d);
            }
            invoke(self.get("_updateRangeCache"), "set")({index, range});
        }
        range.set("start", uniform.get("offset")); range.set("count", uniform.get("itemSize"));
        invoke(self.get("_addedIndices"), "add")({index}); invoke(self.get("updateRanges"), "push")({range});
        return {};
    }
    if (name == "clearUpdateRanges") {
        invoke(self.get("_addedIndices"), "clear")(); superCall(self, "clearUpdateRanges"); return {};
    }
    if (name == "addUniform") { invoke(self.get("uniforms"), "push")({uniform}); return self; }
    if (name == "removeUniform") {
        const auto index = invoke(self.get("uniforms"), "indexOf")({uniform});
        if (!index.strictEquals(-1)) invoke(self.get("uniforms"), "splice")({index, 1});
        return self;
    }
    if (name == "values") {
        if (self.get("_values").isNull()) {
            const auto array = JSValue::intrinsicArrayConstructor(); const auto from = array.get("from");
            self.set("_values", from.call(array, {self.get("buffer")}));
        }
        return self.get("_values");
    }
    if (name == "buffer") {
        auto buffer = self.get("_buffer");
        if (buffer.isNull()) {
            const auto byteLength = self.get("byteLength");
            buffer = NativeArray_detail::constructor(6).construct({nativeArrayBufferConstructor().construct({byteLength})});
            self.set("_buffer", buffer);
        }
        return buffer;
    }
    if (name == "byteLength") {
        const auto bytes = self.get("bytesPerElement"); double offset = 0;
        const auto length = self.get("uniforms").get("length");
        for (double i = 0; i < length.toNumber(); ++i) {
            auto current = self.get("uniforms").get(JSValue(i));
            const auto boundary = current.get("boundary"), size = current.get("itemSize");
            const auto itemSizeLeft = size.toNumber(), itemSizeRight = bytes.toNumber();
            const auto itemSize = itemSizeLeft * itemSizeRight;
            const auto chunkOffset = std::fmod(offset, 16.0);
            const auto chunkPadding = std::fmod(chunkOffset, boundary.toNumber());
            const auto chunkStart = chunkOffset + chunkPadding; offset += chunkPadding;
            if (chunkStart != 0 && 16.0 - chunkStart < itemSize) offset += 16.0 - chunkStart;
            current.set("offset", offset / bytes.toNumber()); current.set("index", i); offset += itemSize;
        }
        return std::ceil(offset / 16.0) * 16.0;
    }
    if (name == "update") {
        bool updated = false; auto iterator = getJSIterator(self.get("uniforms"));
        while (const auto current = stepJSIterator(iterator)) {
            try { if (invoke(self, "updateByType")({*current}).strictEquals(true)) updated = true; }
            catch (...) { closeJSIterator(iterator, std::current_exception()); }
        }
        return updated;
    }
    if (name == "release") { superCall(self, "release"); self.set("_values", nullptr); return {}; }
    if (name == "updateByType") {
        for (const auto* type : {"Number", "Vector2", "Vector3", "Vector4", "Color", "Matrix3", "Matrix4"}) {
            if (uniform.get(std::string("is") + type + "Uniform").toBoolean())
                return invoke(self, (std::string("update") + type).c_str())({uniform});
        }
        ThreeUtils::error({"WebGPUUniformsGroup: Unsupported uniform type.", uniform}); return {};
    }
    if (name == "_getBufferForType") {
        const auto type = arg(args, 0);
        if (type.strictEquals("int") || type.strictEquals("ivec2") || type.strictEquals("ivec3") || type.strictEquals("ivec4"))
            return NativeArray_detail::constructor(4).construct({self.get("buffer").get("buffer")});
        if (type.strictEquals("uint") || type.strictEquals("uvec2") || type.strictEquals("uvec3") || type.strictEquals("uvec4"))
            return NativeArray_detail::constructor(5).construct({self.get("buffer").get("buffer")});
        return self.get("buffer");
    }
    const auto values = self.get("values");
    if (name == "updateNumber") {
        const auto value = invoke(uniform, "getValue")(), offset = uniform.get("offset"), type = invoke(uniform, "getType")();
        if (!values.get(offset).strictEquals(value)) {
            const auto buffer = invoke(self, "_getBufferForType")({type}); assignPair(buffer, values, offset, value);
            invoke(self, "addUniformUpdateRange")({uniform}); return true;
        }
        return false;
    }
    if (name == "updateVector2" || name == "updateVector3" || name == "updateVector4" || name == "updateColor") {
        const auto value = invoke(uniform, "getValue")(), offset = uniform.get("offset");
        const auto type = name == "updateColor" ? JSValue{} : invoke(uniform, "getType")();
        const std::vector<const char*> components = name == "updateColor" ? std::vector<const char*>{"r", "g", "b"}
            : name == "updateVector2" ? std::vector<const char*>{"x", "y"}
            : name == "updateVector3" ? std::vector<const char*>{"x", "y", "z"} : std::vector<const char*>{"x", "y", "z", "w"};
        bool different = false;
        for (std::size_t i = 0; i < components.size(); ++i) {
            const auto left = values.get(add(offset, double(i)));
            if (!left.strictEquals(value.get(components[i]))) { different = true; break; }
        }
        if (!different) return false;
        const auto buffer = name == "updateColor" ? self.get("buffer") : invoke(self, "_getBufferForType")({type});
        for (std::size_t i = 0; i < components.size(); ++i) assignPair(buffer, values, offset, double(i), value, components[i]);
        invoke(self, "addUniformUpdateRange")({uniform}); return true;
    }
    if (name == "updateMatrix3" || name == "updateMatrix4") {
        const auto elements = invoke(uniform, "getValue")().get("elements"), offset = uniform.get("offset");
        if (name == "updateMatrix4") {
            if (arraysEqual(values, elements, offset)) return false;
            const auto buffer = self.get("buffer"); invoke(buffer, "set")({elements, offset}); setArray(values, elements, offset);
        } else {
            bool different = false;
            for (unsigned i = 0; i < 9; ++i) {
                const auto left = values.get(add(offset, double(i + i / 3)));
                if (!left.strictEquals(elements.get(JSValue(i)))) { different = true; break; }
            }
            if (!different) return false;
            const auto buffer = self.get("buffer");
            for (unsigned i = 0; i < 9; ++i) assignMatrixPair(buffer, values, offset, double(i + i / 3), elements, i);
        }
        invoke(self, "addUniformUpdateRange")({uniform}); return true;
    }
    throw std::logic_error("Unknown UniformsGroup method");
}
}

JSValue nativeUniformsGroupConstructor() {
    static thread_local JSWeakValue weak; static thread_local bool initializing = false;
    if (const auto ctor = weak.lock(); ctor && ctor->isConstructible()) return *ctor;
    JSModuleInitializationScope scope(initializing, "UniformsGroup");
    auto state = JSValue::object(nullptr); const auto base = nativeUniformBufferConstructor();
    auto ctor = JSValue::builtinConstructor([](const JSValue& target, const Args& args, const Args& c) -> JSValue {
        auto self = c[0].get("constructor").getPrototype().construct({arg(args, 0)}, target);
        self.set("isUniformsGroup", true); self.set("_values", nullptr); self.set("uniforms", JSValue::array());
        self.set("_updateRangeCache", createNativeMap()); self.set("_addedIndices", createNativeSet()); return self;
    }, {state}, "UniformsGroup", 1);
    state.set("constructor", ctor); ctor.setPrototype(base); auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    // Define members in their source order, including accessor positions.
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
        {"addUniformUpdateRange", 1}, {"clearUpdateRanges", 0}, {"addUniform", 1}, {"removeUniform", 1},
        {"values", 0}, {"buffer", 0}, {"byteLength", 0}, {"update", 0}, {"release", 0}, {"updateByType", 1},
        {"updateNumber", 1}, {"updateVector2", 1}, {"updateVector3", 1}, {"updateVector4", 1}, {"updateColor", 1},
        {"updateMatrix3", 1}, {"updateMatrix4", 1}, {"_getBufferForType", 1}}) {
        JSPropertyDescriptor d; d.configurable = true;
        const bool accessor = std::string(name) == "values" || std::string(name) == "buffer" || std::string(name) == "byteLength";
        const auto function = JSValue::function(method, {name}, accessor ? std::string("get ") + name : name, arity);
        if (accessor) d.getter = function; else { d.value = function; d.writable = true; }
        prototype.defineProperty(name, d);
    }
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
