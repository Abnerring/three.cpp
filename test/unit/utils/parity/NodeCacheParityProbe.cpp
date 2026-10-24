#include "nodes/core/NodeCache.h"
#include "core/JSJobs.h"
#include "core/JSWeakMap.h"
#include <iostream>
#include <string>

namespace {
using Args = std::vector<JSValue>;
using A = JsonValue::Array;
using O = JsonValue::Object;
JSValue call(const JSValue& self, const char* name, const Args& args = {}) {
    return self.get(name).call(self, args);
}
JsonValue encode(const JSValue& value) {
    if (value.isUndefined()) return "#undefined";
    if (value.isCallable()) return "#function";
    if (value.isSymbol()) return "#symbol";
    if (value.isObject()) return "#object";
    return *value.toJSON();
}
void emit(const O& row) { std::cout << JsonValue(row).dump() << '\n'; }
void event(const JSValue& state, const JSValue& item) {
    auto events = state.get("events");
    events.set(std::to_string(events.length()), item);
}
JSValue values(const Args& items) {
    auto result = JSValue::array();
    for (const auto& item : items) result.set(std::to_string(result.length()), item);
    return result;
}
void accessor(JSValue self, const char* key, const JSValue& get, const JSValue& set = {}) {
    JSPropertyDescriptor d; d.getter = get; d.setter = set;
    d.enumerable = d.configurable = true; self.defineProperty(key, d);
}
JsonValue descriptor(const JSPropertyDescriptor& d) {
    O result{{"enumerable", d.enumerable.value_or(false)},
             {"configurable", d.configurable.value_or(false)}};
    if (d.value) {
        result["value"] = encode(*d.value);
        result["writable"] = d.writable.value_or(false);
    } else {
        result["get"] = d.getter && d.getter->isCallable();
        result["set"] = d.setter && d.setter->isCallable();
    }
    return result;
}
JsonValue snapshot(const JSValue& value) {
    O result;
    if (value.isObject()) for (const auto& key : value.ownPropertyNames())
        result[key] = descriptor(*value.getOwnPropertyDescriptor(key));
    return result;
}
template<class F> std::string attempt(F fn) {
    try { fn(); return ""; }
    catch (const JSTypeError&) { return "TypeError"; }
    catch (const JSRangeError&) { return "RangeError"; }
}
void metadata() {
    const auto ctor = NodeCache::constructor(), prototype = NodeCache::prototype();
    auto first = NodeCache().object();
    O methods;
    for (const auto* name : {"getData", "setData"}) {
        const auto fn = prototype.get(name);
        methods[name] = O{{"own", snapshot(fn)},
                          {"descriptor", descriptor(*prototype.getOwnPropertyDescriptor(name))},
                          {"construct", attempt([&] { fn.construct(); })}};
    }
    A keys; for (const auto& key : prototype.ownPropertyNames()) keys.push_back(key);
    emit(O{{"kind", "metadata"}, {"constructor", snapshot(ctor)}, {"prototype", keys},
           {"constructorIdentity", prototype.get("constructor").strictEquals(ctor)},
           {"prototypeIdentity", first.getPrototype().strictEquals(prototype)},
           {"call", attempt([&] { ctor.call(); })}, {"instance", snapshot(first)},
           {"weakMap", isNativeWeakMap(first.get("nodesData"))}, {"methods", methods}});
}
void basic() {
    auto parent = std::make_shared<NodeCache>(); NodeCache cache(parent);
    auto p = parent->object(), c = cache.object(), key = JSValue::object(), other = JSValue::object();
    A rows;
    rows.push_back(encode(cache.getData(key)));
    rows.push_back(encode(parent->setData(key, 17)));
    rows.push_back(encode(cache.getData(key)));
    cache.setData(key, 8); rows.push_back(encode(cache.getData(key)));
    cache.setData(key, JSValue{}); rows.push_back(encode(cache.getData(key)));
    cache.setData(key, nullptr); rows.push_back(encode(cache.getData(key)));
    cache.setData(key, false); rows.push_back(encode(cache.getData(key)));
    rows.push_back(encode(cache.getData(other)));
    c.set("nodesData", createNativeWeakMap()); rows.push_back(encode(cache.getData(key)));
    c.set("parent", nullptr); rows.push_back(encode(cache.getData(key)));
    auto data = JSValue::object(); data.set("tag", 3); cache.setData(key, data);
    const bool identity = cache.getData(key).strictEquals(data);
    c.set("getData", JSValue::function([](const JSValue& self, const Args& a, const Args& cap) -> JSValue {
        return self.strictEquals(cap[0]) && a[0].strictEquals(cap[1]);
    }, {c, key}, "replacement", 1));
    rows.push_back(encode(cache.getData(key)));
    c.set("setData", JSValue::function([](const JSValue& self, const Args& a, const Args& cap) -> JSValue {
        return self.strictEquals(cap[0]) && a[0].strictEquals(cap[1]) && a[1].strictEquals(cap[2]);
    }, {c, key, data}, "replacement", 2));
    rows.push_back(encode(cache.setData(key, data)));
    emit(O{{"kind", "basic"}, {"idGap", cache.id().toNumber() - parent->id().toNumber()},
           {"sameParent", cache.parent().isNull()}, {"dataIdentity", identity}, {"steps", rows}});
}
void keys() {
    for (unsigned kind = 0; kind < 9; ++kind) {
        NodeCache cache;
        const auto key = kind == 0 ? JSValue::object() : kind == 1 ? JSValue::array() :
            kind == 2 ? JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return {};}) :
            kind == 3 ? JSValue{} : kind == 4 ? JSValue(nullptr) : kind == 5 ? JSValue(false) :
            kind == 6 ? JSValue(7) : kind == 7 ? JSValue("key") : JSValue::symbol("key");
        JSValue result;
        const auto setError = attempt([&] { result = cache.setData(key, 23); });
        const auto getError = attempt([&] { result = cache.getData(key); });
        emit(O{{"kind", "keys"}, {"index", kind}, {"setError", setError},
               {"getError", getError}, {"value", encode(result)}});
    }
}
void dynamic(const char* op, unsigned mode) {
    const auto fn = NodeCache::prototype().get(op);
    auto state = JSValue::object(), self = JSValue::object(), map = JSValue::object();
    auto parent = JSValue::object(), key = JSValue::object(), data = JSValue::object();
    state.set("events", JSValue::array()); state.set("mode", mode); state.set("self", self);
    state.set("map", map); state.set("parent", parent); state.set("key", key);
    state.set("data", data); state.set("reads", 0);
    accessor(self, "nodesData", JSValue::function([](const JSValue& receiver,const Args&,const Args& c) {
        const auto s = c[0]; const auto mode = s.get("mode").toNumber();
        event(s, values({"nodesData.get", receiver.strictEquals(s.get("self"))}));
        if (mode == 8) throw JSRangeError("nodesData getter");
        if (mode == 18) return JSValue(nullptr);
        if (mode == 19) {
            auto target = s.get("self"); JSPropertyDescriptor d;
            d.value = JSValue(nullptr); d.writable = d.enumerable = d.configurable = true;
            target.defineProperty("nodesData", d);
        }
        return s.get("map");
    }, {state}));
    accessor(self, "parent", JSValue::function([](const JSValue& receiver,const Args&,const Args& c) {
        auto s = c[0]; const auto mode = s.get("mode").toNumber(), count = s.get("reads").toNumber();
        s.set("reads", count + 1);
        event(s, values({"parent.get", receiver.strictEquals(s.get("self")), count}));
        if (mode == 11 || (mode == 12 && count == 1)) throw JSRangeError("parent getter");
        if (mode == 1 || mode == 5 || mode == 7 || mode == 21) return JSValue(nullptr);
        if (mode == 6 && count > 0) return JSValue(nullptr);
        if (mode == 15) return JSValue{};
        return s.get("parent");
    }, {state}));
    for (const auto* name : {"get", "set"}) {
        accessor(map, name, JSValue::function([](const JSValue& receiver,const Args&,const Args& c) {
            auto s = c[0]; const auto mode = s.get("mode").toNumber(); const auto name = c[1].string();
            event(s, values({name + ".get", receiver.strictEquals(s.get("map"))}));
            if (mode == 9) throw JSRangeError("map method getter");
            if (mode == 16) return JSValue(7);
            return JSValue::function([](const JSValue& receiver,const Args& a,const Args& c) {
                const auto s = c[0]; const auto mode = s.get("mode").toNumber(); const auto name = c[1].string();
                event(s, values({name + ".call", receiver.strictEquals(s.get("map")),
                                a.size(), a[0].strictEquals(s.get("key")),
                                a.size() > 1 && a[1].strictEquals(s.get("data"))}));
                if (mode == 10) throw JSRangeError("map method");
                if (name == "set") return JSValue("ignored-map-result");
                if (mode == 0) return JSValue(9);
                if (mode == 3) return JSValue(nullptr);
                if (mode == 4) return JSValue(false);
                if (mode == 20) {
                    auto target = s.get("parent");
                    target.set("getData", JSValue::function([](const JSValue&,const Args&,const Args& c) {
                        event(c[0], "replacement.call"); return JSValue(99);
                    }, {s}));
                }
                return JSValue{};
            }, {s, c[1]}, name, name == "get" ? 1 : 2);
        }, {state, name}));
    }
    if (mode != 20) accessor(parent, "getData", JSValue::function([](const JSValue& receiver,const Args&,const Args& c) {
        const auto s = c[0]; const auto mode = s.get("mode").toNumber();
        event(s, values({"getData.get", receiver.strictEquals(s.get("parent"))}));
        if (mode == 13) throw JSRangeError("parent method getter");
        if (mode == 17) return JSValue(7);
        return JSValue::function([](const JSValue& receiver,const Args& a,const Args& c) {
            const auto s = c[0];
            event(s, values({"getData.call", receiver.strictEquals(s.get("parent")), a.size(),
                            a[0].strictEquals(s.get("key"))}));
            if (s.get("mode").strictEquals(14)) throw JSRangeError("parent method");
            return JSValue(31);
        }, {s}, "getData", 1);
    }, {state}));
    JSValue result;
    const auto error = attempt([&] { result = fn.call(self, {key, data}); });
    emit(O{{"kind", "dynamic"}, {"op", op}, {"mode", mode}, {"error", error},
           {"result", encode(result)}, {"events", *state.get("events").toJSON()}});
}
void construct(unsigned mode) {
    const auto ctor = NodeCache::constructor();
    const auto before = ctor.construct().get("id").toNumber();
    auto state = JSValue::object(), prototype = JSValue::object();
    state.set("events", JSValue::array()); state.set("prototype", prototype); state.set("mode", mode);
    auto other = JSValue::constructor([](const JSValue&,const Args&,const Args&)->JSValue{return {};},
                                     {}, "Other", 0, JSValue::ConstructorKind::Callable);
    other.set("prototype", prototype);
    if (mode == 8 || mode == 12) for (const auto* name : {"id", "nodesData", "parent"}) {
        accessor(prototype, name, {}, JSValue::function([](const JSValue& receiver,const Args& a,const Args& c) {
            const auto s = c[0]; const auto name = c[1].string();
            const auto value = name == "id" ? JSValue("id") : a[0].isObject() ? JSValue("object") : a[0];
            event(s, values({name + ".set", receiver.getPrototype().strictEquals(s.get("prototype")), value}));
            if (s.get("mode").strictEquals(12) && name == "id")
                event(s, values({"nested", NodeCache().id().toNumber() - a[0].toNumber()}));
            return JSValue{};
        }, {state, name}, "set", 1));
    }
    if (mode >= 9 && mode <= 11) {
        JSPropertyDescriptor d; d.value = 77; d.enumerable = d.configurable = true;
        prototype.defineProperty(mode == 9 ? "id" : mode == 10 ? "nodesData" : "parent", d);
    }
    const auto parent = mode == 3 ? JSValue(false) : mode == 4 ? JSValue(0) :
        mode == 5 ? JSValue("text") : mode == 6 ? JSValue::object() : JSValue(nullptr);
    Args args = mode == 0 ? Args{} : mode == 1 ? Args{JSValue{}} : Args{parent};
    JSValue result;
    const auto error = attempt([&] { result = mode >= 7 ? ctor.construct(args, other) : ctor.construct(args); });
    const auto after = ctor.construct().get("id").toNumber();
    auto own = snapshot(result);
    if (result.isObject() && result.hasOwn("id")) own["id"]["value"] = result.get("id").toNumber() - before;
    emit(O{{"kind", "construct"}, {"mode", mode}, {"error", error}, {"own", own},
           {"samePrototype", result.isObject() && result.getPrototype().strictEquals(prototype)},
           {"sameParent", result.isObject() && result.get("parent").strictEquals(parent)},
           {"idAdvance", after - before}, {"events", *state.get("events").toJSON()}});
}
void gc(const std::string& mode) {
    const auto realm = JSValue::object();
    JSValue retained = JSValue::array();
    JSWeakValue cacheRef, mapRef, keyRef, valueRef, parentRef;
    {
        auto parent = NodeCache().object(), cache = NodeCache(parent).object();
        auto map = cache.get("nodesData"), key = JSValue::object(), value = JSValue::object();
        cacheRef = JSWeakValue(cache); mapRef = JSWeakValue(map); keyRef = JSWeakValue(key);
        valueRef = JSWeakValue(value); parentRef = JSWeakValue(parent);
        call(cache, "setData", {key, value});
        if (mode == "retain-cache" || mode == "retain-key" || mode == "replace-map" || mode == "delete")
            retained.set("0", cache);
        if (mode == "retain-key" || mode == "key-only" || mode == "replace-map" || mode == "delete")
            retained.set("1", key);
        if (mode == "value-only") retained.set("0", value);
        if (mode == "cycle" || mode == "retain-cache") { value.set("key", key); value.set("cache", cache); }
        if (mode == "replace-map") cache.set("nodesData", createNativeWeakMap());
        if (mode == "delete") call(map, "delete", {key});
    }
    endJSJob(); realm.collectGarbage();
    emit(O{{"mode", mode}, {"cache", !cacheRef.expired()}, {"map", !mapRef.expired()},
           {"key", !keyRef.expired()}, {"value", !valueRef.expired()},
           {"parent", !parentRef.expired()}, {"retained", retained.length()}});
}
} // namespace
int main(int argc, char** argv) {
    if (argc > 1) { gc(argv[1]); return 0; }
    const auto realm = JSValue::object();
    metadata(); basic(); keys();
    for (const auto* op : {"getData", "setData"}) for (unsigned mode = 0; mode < 22; ++mode) {
        dynamic(op, mode); endJSJob(); realm.collectGarbage();
    }
    for (unsigned mode = 0; mode < 13; ++mode) construct(mode);
}
