#include "animation/AnimationObjectGroup.h"
#include "animation/AnimationRuntime.h"
#include "animation/PropertyBinding.h"
#include "math/MathUtils.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue(); }
JSValue get(const JSValue& object, const JSValue& key) { return object.get(key.toString()); }
void set(JSValue object, const JSValue& key, const JSValue& value) { object.set(key.toString(), value); }
void erase(JSValue object, const JSValue& key) {
    if (!object.erase(key.toString())) throw JSTypeError("Cannot delete nonconfigurable UUID index");
}
void pop(JSValue array) { const auto method = array.get("pop"); method.call(array); }
void push(JSValue array, const JSValue& value) { const auto method = array.get("push"); method.call(array, {value}); }
JSValue increment(JSValue& value) { const auto old = value.toNumber(); value = old + 1; return old; }
JSValue decrement(JSValue& value) { value = value.toNumber() - 1; return value; }

// Relational comparison uses the same left-to-right coercion as the operators.
// UTF-16 code units determine string order, including lone surrogate strings.
std::vector<std::uint16_t> units(const std::string& text) {
    std::vector<std::uint16_t> result;
    for (std::size_t i = 0; i < text.size();) {
        const auto first = static_cast<unsigned char>(text[i++]);
        std::uint32_t value = first;
        int continuation = first < 0x80 ? 0 : first < 0xe0 ? 1 : first < 0xf0 ? 2 : 3;
        if (continuation) value &= (1u << (6 - continuation)) - 1;
        while (continuation-- > 0) value = (value << 6) | (static_cast<unsigned char>(text[i++]) & 63);
        if (value > 0xffff) { value -= 0x10000; result.push_back(std::uint16_t(0xd800 + (value >> 10))); result.push_back(std::uint16_t(0xdc00 + (value & 1023))); }
        else result.push_back(std::uint16_t(value));
    }
    return result;
}
bool compare(const JSValue& left, const JSValue& right, bool greaterEqual = false) {
    const auto a = left.toPrimitive(JSValue::PreferredType::Number);
    const auto b = right.toPrimitive(JSValue::PreferredType::Number);
    if (a.type() == JSValue::Type::String && b.type() == JSValue::Type::String)
        return greaterEqual ? units(a.string()) >= units(b.string()) : units(a.string()) < units(b.string());
    const auto x = a.toNumber(), y = b.toNumber();
    return greaterEqual ? x >= y : x < y;
}
JSValue binding(const JSValue& object, const JSValue& path, const JSValue& parsed) {
    return nativePropertyBinding(std::make_shared<PropertyBinding>(object, path, parsed));
}
JSValue arrayConstructor(const JSValue& length) {
    if (length.type() != JSValue::Type::Number) {
        auto array = JSValue::array(1); array.set("0", length); return array;
    }
    const auto n = length.number();
    if (!std::isfinite(n) || n < 0 || std::floor(n) != n || n > 4294967295.) throw JSRangeError("Invalid array length");
    return JSValue::array(static_cast<std::uint32_t>(n));
}
JSValue groupAdd(const JSValue& receiver, const Args& args, const Args&) {
    auto self = receiver;
    const auto objects = self.get("_objects"), indices = self.get("_indicesByUUID"),
        paths = self.get("_paths"), parsedPaths = self.get("_parsedPaths"), bindings = self.get("_bindings"), nBindings = bindings.get("length");
    JSValue known, nObjects = objects.get("length"), nCached = self.get("nCachedObjects_");
    for (const auto& object : args) {
        const auto uuid = object.get("uuid"); auto index = get(indices, uuid);
        if (index.isUndefined()) {
            index = increment(nObjects); set(indices, uuid, index); push(objects, object);
            for (JSValue j = 0; !j.strictEquals(nBindings); increment(j)) {
                const auto list = get(bindings, j), method = list.get("push");
                const auto path = get(paths, j), parsed = get(parsedPaths, j);
                const auto created = binding(object, path, parsed); method.call(list, {created});
            }
        } else if (compare(index, nCached)) {
            known = get(objects, index);
            const auto firstActive = decrement(nCached), lastCachedObject = get(objects, firstActive);
            set(indices, lastCachedObject.get("uuid"), index); set(objects, index, lastCachedObject);
            set(indices, uuid, firstActive); set(objects, firstActive, object);
            for (JSValue j = 0; !j.strictEquals(nBindings); increment(j)) {
                const auto list = get(bindings, j), lastCached = get(list, firstActive);
                auto current = get(list, index); set(list, index, lastCached);
                if (current.isUndefined()) {
                    const auto path = get(paths, j), parsed = get(parsedPaths, j); current = binding(object, path, parsed);
                }
                set(list, firstActive, current);
            }
        } else if (!get(objects, index).strictEquals(known)) {
            std::cerr << "THREE.AnimationObjectGroup: Different objects with the same UUID detected. Clean the caches or recreate your infrastructure when reloading scenes.\n";
        }
    }
    self.set("nCachedObjects_", nCached); return {};
}
JSValue groupRemove(const JSValue& receiver, const Args& args, const Args&) {
    auto self = receiver;
    const auto objects = self.get("_objects"), indices = self.get("_indicesByUUID"),
        bindings = self.get("_bindings"), nBindings = bindings.get("length");
    auto nCached = self.get("nCachedObjects_");
    for (const auto& object : args) {
        const auto uuid = object.get("uuid"), index = get(indices, uuid);
        if (!index.isUndefined() && compare(index, nCached, true)) {
            const auto lastCached = increment(nCached), firstActiveObject = get(objects, lastCached);
            set(indices, firstActiveObject.get("uuid"), index); set(objects, index, firstActiveObject);
            set(indices, uuid, lastCached); set(objects, lastCached, object);
            for (JSValue j = 0; !j.strictEquals(nBindings); increment(j)) {
                const auto list = get(bindings, j), firstActive = get(list, lastCached), current = get(list, index);
                set(list, index, firstActive); set(list, lastCached, current);
            }
        }
    }
    self.set("nCachedObjects_", nCached); return {};
}
JSValue groupUncache(const JSValue& receiver, const Args& args, const Args&) {
    auto self = receiver;
    const auto objects = self.get("_objects"), indices = self.get("_indicesByUUID"),
        bindings = self.get("_bindings"), nBindings = bindings.get("length");
    auto nCached = self.get("nCachedObjects_"), nObjects = objects.get("length");
    for (const auto& object : args) {
        const auto uuid = object.get("uuid"), index = get(indices, uuid);
        if (index.isUndefined()) continue;
        erase(indices, uuid);
        if (compare(index, nCached)) {
            const auto firstActive = decrement(nCached), lastCachedObject = get(objects, firstActive),
                lastIndex = decrement(nObjects), lastObject = get(objects, lastIndex);
            if (!index.strictEquals(firstActive)) set(indices, lastCachedObject.get("uuid"), index);
            set(objects, index, lastCachedObject);
            if (!firstActive.strictEquals(lastIndex)) set(indices, lastObject.get("uuid"), firstActive);
            set(objects, firstActive, lastObject); pop(objects);
            for (JSValue j = 0; !j.strictEquals(nBindings); increment(j)) {
                const auto list = get(bindings, j), lastCached = get(list, firstActive), last = get(list, lastIndex);
                set(list, index, lastCached); set(list, firstActive, last); pop(list);
            }
        } else {
            const auto lastIndex = decrement(nObjects), lastObject = get(objects, lastIndex);
            if (!index.strictEquals(lastIndex)) set(indices, lastObject.get("uuid"), index);
            set(objects, index, lastObject); pop(objects);
            for (JSValue j = 0; !j.strictEquals(nBindings); increment(j)) {
                const auto list = get(bindings, j), last = get(list, lastIndex);
                set(list, index, last); pop(list);
            }
        }
    }
    self.set("nCachedObjects_", nCached); return {};
}
JSValue subscribe(const JSValue& receiver, const Args& args, const Args&) {
    auto self = receiver;
    const auto path = arg(args, 0), parsedPath = arg(args, 1), indices = self.get("_bindingsIndicesByPath");
    auto index = get(indices, path); const auto bindings = self.get("_bindings");
    if (!index.isUndefined()) return get(bindings, index);
    const auto paths = self.get("_paths"), parsedPaths = self.get("_parsedPaths"), objects = self.get("_objects"),
        nObjects = objects.get("length"), nCached = self.get("nCachedObjects_"), list = arrayConstructor(nObjects);
    index = bindings.get("length"); set(indices, path, index);
    push(paths, path); push(parsedPaths, parsedPath); push(bindings, list);
    const auto n = objects.get("length");
    for (auto i = nCached; !i.strictEquals(n); increment(i)) {
        const auto object = get(objects, i), created = binding(object, path, parsedPath); set(list, i, created);
    }
    return list;
}
JSValue unsubscribe(const JSValue& receiver, const Args& args, const Args&) {
    auto self = receiver;
    const auto path = arg(args, 0), indices = self.get("_bindingsIndicesByPath"), index = get(indices, path);
    if (!index.isUndefined()) {
        const auto paths = self.get("_paths"), parsedPaths = self.get("_parsedPaths"), bindings = self.get("_bindings"),
            lastIndex = JSValue(bindings.get("length").toNumber() - 1), last = get(bindings, lastIndex), lastPath = get(paths, lastIndex);
        set(indices, lastPath, index); set(bindings, index, last); pop(bindings);
        const auto parsed = get(parsedPaths, lastIndex); set(parsedPaths, index, parsed); pop(parsedPaths);
        const auto lastName = get(paths, lastIndex); set(paths, index, lastName); pop(paths);
    }
    return {};
}
void method(JSValue object, const char* name, JSValue::NativeFunction function, std::uint32_t arity) {
    JSPropertyDescriptor descriptor; descriptor.value = JSValue::function(function, {}, name, arity);
    descriptor.writable = true; descriptor.configurable = true; object.defineProperty(name, descriptor);
}
void getter(JSValue object, const char* name, JSValue::NativeFunction function, JSValue scope) {
    JSPropertyDescriptor descriptor; descriptor.getter = JSValue::function(function, {scope}, std::string("get ") + name, 0);
    descriptor.enumerable = true; descriptor.configurable = true; object.defineProperty(name, descriptor);
}
}

JSValue AnimationObjectGroup::prototype() {
    static thread_local auto value = [] {
        auto p = JSValue::object();
        JSPropertyDescriptor constructor; constructor.value = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue {
            throw JSTypeError("Class constructor AnimationObjectGroup cannot be invoked without 'new'");
        }, {}, "AnimationObjectGroup", 0); constructor.writable = true; constructor.enumerable = false; constructor.configurable = true;
        p.defineProperty("constructor", constructor);
        JSPropertyDescriptor proto; proto.value = p; proto.writable = proto.enumerable = proto.configurable = false;
        constructor.value->defineProperty("prototype", proto);
        method(p, "add", groupAdd, 0); method(p, "remove", groupRemove, 0); method(p, "uncache", groupUncache, 0);
        method(p, "subscribe_", subscribe, 2); method(p, "unsubscribe_", unsubscribe, 1);
        return p;
    }();
    return value;
}
AnimationObjectGroup::AnimationObjectGroup(const std::vector<JSValue>& arguments) : object_(JSValue::object(prototype())) {
    const auto args = arguments;
    auto self = object_; self.set("isAnimationObjectGroup", true); self.set("uuid", MathUtils::generateUUID());
    const auto slice = JSValue::array().getPrototype().get("slice"), call = slice.get("call");
    auto argumentObject = JSValue::object();
    JSPropertyDescriptor length; length.value = double(args.size()); length.writable = true; length.configurable = true;
    argumentObject.defineProperty("length", length);
    for (std::size_t i = 0; i < args.size(); ++i) argumentObject.set(std::to_string(i), args[i]);
    const auto objects = call.call(slice, {argumentObject});
    self.set("_objects", objects); self.set("nCachedObjects_", 0);
    auto indices = JSValue::object(); self.set("_indicesByUUID", indices);
    for (std::size_t i = 0; i < args.size(); ++i) set(indices, args[i].get("uuid"), double(i));
    self.set("_paths", JSValue::array()); self.set("_parsedPaths", JSValue::array());
    self.set("_bindings", JSValue::array()); self.set("_bindingsIndicesByPath", JSValue::object());
    auto stats = JSValue::object(), counts = JSValue::object();
    getter(counts, "total", [](const JSValue&, const Args&, const Args& c) { return c[0].get("_objects").get("length"); }, self);
    getter(counts, "inUse", [](const JSValue& receiver, const Args&, const Args& c) {
        const auto total = receiver.get("total"), cached = c[0].get("nCachedObjects_");
        return JSValue(Interpolant_detail::subtract(total, cached));
    }, self);
    stats.set("objects", counts);
    getter(stats, "bindingsPerObject", [](const JSValue&, const Args&, const Args& c) { return c[0].get("_bindings").get("length"); }, self);
    self.set("stats", stats);
}
AnimationObjectGroup AnimationObjectGroup::fromObject(JSValue object) { return AnimationObjectGroup(std::move(object), Existing{}); }
JSValue AnimationObjectGroup::add(const std::vector<JSValue>& objects) { const auto args = objects; return groupAdd(object_, args, {}); }
JSValue AnimationObjectGroup::remove(const std::vector<JSValue>& objects) { const auto args = objects; return groupRemove(object_, args, {}); }
JSValue AnimationObjectGroup::uncache(const std::vector<JSValue>& objects) { const auto args = objects; return groupUncache(object_, args, {}); }
JSValue AnimationObjectGroup::subscribe_(JSValue path, JSValue parsedPath) { return subscribe(object_, {path, parsedPath}, {}); }
JSValue AnimationObjectGroup::unsubscribe_(JSValue path) { return unsubscribe(object_, {path}, {}); }

