// Direct port of three.js WebGLGeometries.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See THREEJS-LICENSE.
#include "renderers/webgl/WebGLGeometries.h"
#include "core/BufferAttribute.h"
#include "core/JSWeakMap.h"
#include "core/JSWeakRef.h"
#include "core/JSFinalizationRegistry.h"
#include "core/NativeObject.h"
#include <cmath>
#include <unordered_set>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
void literal(JSValue object, const char* key, JSValue value) {
    JSPropertyDescriptor d; d.value = std::move(value); d.writable = d.enumerable = d.configurable = true;
    object.defineProperty(key, d);
}
std::string key(const JSValue& value) { return value.toPrimitive(JSValue::PreferredType::String).toString(); }
// For-in snapshots each object's own string keys, suppressing non-enumerable
// shadows and checking deletion before each visit. Symbols are not runtime keys.
class ForIn {
    JSValue object_;
    std::vector<std::string> names_;
    std::size_t index_ = 0;
    std::unordered_set<std::string> visited_;
  public:
    explicit ForIn(JSValue object) : object_(std::move(object)) {
        if (object_.isObject()) names_ = object_.ownPropertyNames();
        else if (object_.type() == JSValue::Type::String) names_ = object_.ownPropertyNames();
        else object_ = nullptr;
    }
    std::optional<std::string> next() {
        while (!object_.isNull() && !object_.isUndefined()) {
            while (index_ < names_.size()) {
                auto name = names_[index_++];
                if (visited_.contains(name)) continue;
                const auto descriptor = object_.getOwnPropertyDescriptor(name);
                if (!descriptor) continue;
                visited_.insert(name);
                if (descriptor->enumerable.value_or(false)) return name;
            }
            object_ = object_.getPrototype(); index_ = 0;
            if (!object_.isNull()) names_ = object_.ownPropertyNames();
        }
        return {};
    }
};
std::u16string utf16(const std::string& value) {
    std::u16string result;
    for (std::size_t i = 0; i < value.size();) {
        auto code = std::uint32_t(static_cast<unsigned char>(value[i++]));
        if (code >= 0xc0) {
            const auto n = code >= 0xf0 ? 3 : code >= 0xe0 ? 2 : 1;
            code &= n == 3 ? 7 : n == 2 ? 15 : 31;
            for (int j = 0; j < n; ++j) code = (code << 6) | (static_cast<unsigned char>(value[i++]) & 63);
        }
        if (code > 0xffff) { code -= 0x10000; result.push_back(char16_t(0xd800 + (code >> 10))); result.push_back(char16_t(0xdc00 + (code & 1023))); }
        else result.push_back(char16_t(code));
    }
    return result;
}
bool less(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(JSValue::PreferredType::Number), b = right.toPrimitive(JSValue::PreferredType::Number);
    if (a.type() == JSValue::Type::String && b.type() == JSValue::Type::String) return utf16(a.string()) < utf16(b.string());
    const auto x = a.toNumber(); const auto y = b.toNumber(); return x < y;
}
void adjustMemory(const JSValue& info, double increment) {
    // A postfix update resolves its reference before reading/coercing its value.
    auto memory = info.get("memory"); const auto number = memory.get("geometries").toNumber();
    memory.set("geometries", number + increment);
}
void destroyGeometry(JSValue geometry, const JSValue& state, const JSValue& attributes,
                     const JSValue& bindingStates, const JSValue& info) {
    if (!geometry.get("index").isNull()) {
        const auto remove = attributes.get("remove"), index = geometry.get("index"); remove.call(attributes, {index});
    }
    ForIn names(geometry.get("attributes"));
    while (const auto name = names.next()) {
        const auto remove = attributes.get("remove"), attribute = geometry.get("attributes").get(*name);
        remove.call(attributes, {attribute});
    }
    const auto removeListener = geometry.get("removeEventListener");
    removeListener.call(geometry, {"dispose", state.get("onGeometryDispose")});
    auto geometries = state.get("geometries"); geometries.erase(key(geometry.get("id")));
    const auto registry = state.get("registry"), unregister = registry.get("unregister"); unregister.call(registry, {geometry});
    const auto wires = state.get("wireframeAttributes"), get = wires.get("get"), attribute = get.call(wires, {geometry});
    if (attribute.toBoolean()) {
        const auto remove = attributes.get("remove"); remove.call(attributes, {attribute});
        const auto erase = wires.get("delete"); erase.call(wires, {geometry});
    }
    const auto release = bindingStates.get("releaseStatesOfGeometry"); release.call(bindingStates, {geometry});
    if (geometry.get("isInstancedBufferGeometry").strictEquals(true)) geometry.erase("_maxInstanceCount");
    adjustMemory(info, -1);
}
void updateWireframe(const JSValue& geometry, const JSValue& state, const JSValue& attributes) {
    const auto geometryIndex = geometry.get("index");
    const auto position = geometry.get("attributes").get("position");
    JSValue version = 0;
    if (position.isUndefined()) return;
    const auto count = !geometryIndex.isNull() ? geometryIndex.get("array").get("length")
        : JSValue(position.get("array").get("length").toNumber() / 3 - 1);
    const auto wide = position.get("count").toNumber() >= 65535;
    const auto size = std::ceil(count.toNumber() / 3) * 6;
    const auto attribute = wide ? nativeAttribute(std::make_shared<Uint32BufferAttribute>(size, 1))
        : nativeAttribute(std::make_shared<Uint16BufferAttribute>(size, 1));
    auto indices = attribute.get("array");
    double j = 0;
    if (!geometryIndex.isNull()) {
        const auto array = geometryIndex.get("array"); version = geometryIndex.get("version");
        for (double i = 0; less(i, count); i += 3) {
            const auto a = array.get(JSValue(i).toString()), b = array.get(JSValue(i + 1).toString()), c = array.get(JSValue(i + 2).toString());
            for (const auto& value : {a, b, b, c, c, a}) indices.set(JSValue(j++).toString(), value);
        }
    } else {
        version = position.get("version");
        for (double i = 0; less(i, count); i += 3) {
            for (const auto value : {i, i + 1, i + 1, i + 2, i + 2, i}) indices.set(JSValue(j++).toString(), value);
        }
    }
    auto mutableAttribute = attribute; mutableAttribute.set("version", version);
    const auto wires = state.get("wireframeAttributes"), get = wires.get("get"), previous = get.call(wires, {geometry});
    if (previous.toBoolean()) { const auto remove = attributes.get("remove"); remove.call(attributes, {previous}); }
    const auto set = wires.get("set"); set.call(wires, {geometry, attribute});
}
JSValue method(const JSValue&, const Args& args, const Args& captures) {
    const auto& name = captures[0].string();
    const auto state = captures[1], gl = captures[2], attributes = captures[3], info = captures[4], bindingStates = captures[5];
    if (name == "onGeometryDispose") { destroyGeometry(arg(args, 0).get("target"), state, attributes, bindingStates, info); return {}; }
    if (name == "dispose") {
        const auto geometries = state.get("geometries"); ForIn ids(geometries);
        while (const auto id = ids.next()) {
            const auto reference = geometries.get(*id), deref = reference.get("deref"), geometry = deref.call(reference);
            if (!geometry.isUndefined()) destroyGeometry(geometry, state, attributes, bindingStates, info);
        }
        return {};
    }
    const auto geometry = arg(args, name == "get" ? 1 : 0);
    if (name == "get") {
        auto geometries = state.get("geometries");
        if (!geometries.get(key(geometry.get("id"))).isUndefined()) return geometry;
        const auto add = geometry.get("addEventListener"); add.call(geometry, {"dispose", state.get("onGeometryDispose")});
        const auto id = key(geometry.get("id")); const auto reference = nativeWeakRefConstructor().construct({geometry}); geometries.set(id, reference);
        const auto registry = state.get("registry"), registerTarget = registry.get("register"), held = geometry.get("id");
        registerTarget.call(registry, {geometry, held, geometry}); adjustMemory(info, 1); return geometry;
    }
    if (name == "update") {
        const auto geometryAttributes = geometry.get("attributes"); ForIn names(geometryAttributes);
        while (const auto name = names.next()) {
            const auto update = attributes.get("update"), attribute = geometryAttributes.get(*name), target = gl.get("ARRAY_BUFFER");
            update.call(attributes, {attribute, target});
        }
        return {};
    }
    const auto wires = state.get("wireframeAttributes"), get = wires.get("get"), current = get.call(wires, {geometry});
    if (current.toBoolean()) {
        const auto index = geometry.get("index");
        if (!index.isNull()) {
            const auto previous = current.get("version"), next = index.get("version");
            if (less(previous, next)) updateWireframe(geometry, state, attributes);
        }
    } else updateWireframe(geometry, state, attributes);
    const auto latestGet = wires.get("get"); return latestGet.call(wires, {geometry});
}
}
WebGLGeometries::WebGLGeometries(JSValue gl, JSValue attributes, JSValue info, JSValue bindingStates) : object_(JSValue::object()) {
    auto state = JSValue::object(nullptr), geometries = JSValue::object();
    state.set("geometries", geometries); state.set("wireframeAttributes", createNativeWeakMap());
    const auto cleanup = JSValue::function(+[](const JSValue&, const Args& args, const Args& captures)->JSValue {
        auto map = captures[0]; map.erase(key(arg(args, 0))); return {};
    }, {geometries}, "", 1);
    state.set("registry", nativeFinalizationRegistryConstructor().construct({cleanup}));
    state.set("onGeometryDispose", JSValue::constructor(method, {"onGeometryDispose", state, gl, attributes, info, bindingStates}, "onGeometryDispose", 1, JSValue::ConstructorKind::Callable));
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"get",2},{"update",1},{"getWireframeAttribute",1},{"dispose",0}})
        literal(object_, name, JSValue::constructor(method, {name, state, gl, attributes, info, bindingStates}, name, arity, JSValue::ConstructorKind::Callable));
}
JSValue WebGLGeometries::get(JSValue object, JSValue geometry) { return object_.get("get").call(object_, {object, geometry}); }
JSValue WebGLGeometries::update(JSValue geometry) { return object_.get("update").call(object_, {geometry}); }
JSValue WebGLGeometries::getWireframeAttribute(JSValue geometry) { return object_.get("getWireframeAttribute").call(object_, {geometry}); }
JSValue WebGLGeometries::dispose() { return object_.get("dispose").call(object_); }
