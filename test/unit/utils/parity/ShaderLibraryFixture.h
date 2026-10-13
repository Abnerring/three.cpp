#pragma once
#include "core/JSValue.h"
#include "renderers/shaders/ShaderChunk.h"
#include "renderers/shaders/ShaderLib.h"
#include "renderers/shaders/UniformsLib.h"
#include "renderers/shaders/UniformsUtils.h"
#include <cmath>
struct Graph {
    std::vector<JSValue> seen;
    JsonValue encode(const JSValue& v) {
        if (v.isUndefined())
            return "$undefined";
        if (v.isCallable())
            return "$function";
        if (!v.isObject())
            return *v.toJSON();
        for (size_t i = 0; i < seen.size(); ++i)
            if (v.sameValue(seen[i]))
                return JsonValue::Object{{"ref", double(i)}};
        const auto id = seen.size();
        seen.push_back(v);
        JsonValue::Object out{{"id", double(id)}};
        for (const auto* type :
             {"Color", "Vector2", "Vector3", "Vector4", "Matrix3", "Matrix4", "Quaternion"}) {
            if (v.get(std::string("is") + type).toBoolean()) {
                out["type"] = type;
                JsonValue::Array values;
                const std::string t = type;
                if (t == "Color")
                    for (const auto* k : {"r", "g", "b"})
                        values.push_back(encode(v.get(k)));
                else if (t.starts_with("Matrix")) {
                    const auto a = v.get("elements");
                    for (size_t i = 0; i < a.length(); ++i)
                        values.push_back(encode(a.get(std::to_string(i))));
                } else {
                    for (const auto* k : {"x", "y"})
                        values.push_back(encode(v.get(k)));
                    if (t != "Vector2")
                        values.push_back(encode(v.get("z")));
                    if (t == "Vector4" || t == "Quaternion")
                        values.push_back(encode(v.get("w")));
                }
                out["values"] = values;
                return out;
            }
        }
        out["type"] = v.type() == JSValue::Type::Array ? "Array" : "Object";
        JsonValue::Array fields;
        for (const auto& k : v.ownPropertyNames()) {
            const auto d = *v.getOwnPropertyDescriptor(k);
            JsonValue::Object f{{"key", k},
                                {"enumerable", d.enumerable.value_or(false)},
                                {"configurable", d.configurable.value_or(false)}};
            if (d.value) {
                f["writable"] = d.writable.value_or(false);
                f["value"] = encode(*d.value);
            } else {
                f["get"] = d.getter && d.getter->isCallable();
                f["set"] = d.setter && d.setter->isCallable();
            }
            fields.emplace_back(f);
        }
        out["fields"] = fields;
        return out;
    }
};
using Args = std::vector<JSValue>;
inline void literal(JSValue o, const std::string& k, const JSValue& v) {
    JSPropertyDescriptor d;
    d.value = v;
    d.writable = d.enumerable = d.configurable = true;
    o.defineProperty(k, d);
}
inline JSValue obj(std::initializer_list<std::pair<const char*, JSValue>> f = {}) {
    auto o = JSValue::object();
    for (const auto& [k, v] : f)
        literal(o, k, v);
    return o;
}
inline JSValue arr(std::initializer_list<JSValue> vs = {}) {
    auto a = JSValue::array();
    size_t i = 0;
    for (const auto& v : vs)
        literal(a, std::to_string(i++), v);
    return a;
}
inline void accessor(JSValue o, const std::string& k, const JSValue& get, const JSValue& set = {}) {
    JSPropertyDescriptor d;
    d.getter = get;
    d.setter = set;
    d.enumerable = d.configurable = true;
    o.defineProperty(k, d);
}
inline void event(JSValue s, const JSValue& label) {
    auto trace = s.get("trace");
    literal(trace, std::to_string(trace.length()), label);
    if (trace.length() == s.get("throwAt").toNumber())
        throw JSRangeError("audit");
}
inline JsonValue libraryInitCase(int mode, int point) {
    auto uniforms = nativeUniformsLib(), chunks = nativeShaderChunk();
    const auto common = uniforms.get("common");
    auto s = obj({{"trace", arr()}, {"throwAt", point}, {"mode", mode}, {"busy", false}});
    auto objectPrototype = JSValue::object().getPrototype();
    for (const auto& name : uniforms.keys()) {
        const auto original = uniforms.get(name);
        accessor(uniforms, name,
                 JSValue::function(
                     [](const JSValue&, const Args&, const Args& c) -> JSValue {
                         event(c[0], "group:" + c[1].toString());
                         return c[2];
                     },
                     {s, name, original}));
    }
    for (const auto& name : chunks.keys()) {
        const auto original = chunks.get(name);
        accessor(chunks, name,
                 JSValue::function(
                     [](const JSValue&, const Args&, const Args& c) -> JSValue {
                         event(c[0], "chunk:" + c[1].toString());
                         return c[2];
                     },
                     {s, name, original}));
    }
    s.get("trace").set("length", 0);
    auto cloneable = obj();
    accessor(cloneable, "isColor",
             JSValue::function(
                 [](const JSValue&, const Args&, const Args& c) -> JSValue {
                     event(c[0], "flag");
                     return true;
                 },
                 {s}));
    accessor(cloneable, "isRenderTargetTexture",
             JSValue::function(
                 [](const JSValue&, const Args&, const Args& c) -> JSValue {
                     event(c[0], "target");
                     return false;
                 },
                 {s}));
    accessor(cloneable, "clone",
             JSValue::function(
                 [](const JSValue& self, const Args&, const Args& c) -> JSValue {
                     event(c[0], "get-clone");
                     return JSValue::function(
                         [](const JSValue& self, const Args& a, const Args& c) -> JSValue {
                             event(c[0], "clone");
                             const int mode = int(c[0].get("mode").number());
                             if (mode == 2) {
                                 auto nested =
                                     mergeUniforms(arr({obj({{"x", obj({{"value", 3}})}})}));
                                 event(c[0], nested.get("x").get("value"));
                             }
                             if (mode == 3) {
                                 auto common = c[1];
                                 common.set("late", obj({{"value", 42}}));
                             }
                             return obj({{"token", 17}});
                         },
                         {c[0], c[1]});
                 },
                 {s, common}));
    literal(common, "audit", obj({{"value", cloneable}}));
    if (mode == 1 || mode == 4)
        for (const auto* name : {"basic", "uniforms", "vertexShader", "fragmentShader", "physical"})
            accessor(objectPrototype, name, {},
                     JSValue::function(
                         [](const JSValue& self, const Args& a, const Args& c) -> JSValue {
                             event(c[0], "set:" + c[1].toString());
                             literal(self, c[1].toString(), a[0]);
                             return JSValue();
                         },
                         {s, name}));
    if (mode == 1)
        for (const auto* name :
             {"basic", "uniforms", "vertexShader", "fragmentShader", "physical"}) {
            JSPropertyDescriptor d;
            d.enumerable = false;
            objectPrototype.defineProperty(name, d);
        }
    JSValue result;
    std::string error;
    try {
        result = nativeShaderLib();
    } catch (const JSTypeError&) {
        error = "TypeError";
    } catch (const JSRangeError&) {
        error = "RangeError";
    }
    for (const auto* name : {"basic", "uniforms", "vertexShader", "fragmentShader", "physical"})
        if (mode == 1 || mode == 4)
            objectPrototype.erase(name);
    // Stop observing before serializing the result; descriptors are not evaluated.
    Graph g;
    return JsonValue::Object{{"kind", "init"},
                             {"mode", mode},
                             {"point", point},
                             {"error", error},
                             {"trace", s.get("trace").toJSON().value()},
                             {"result", g.encode(result)}};
}
// Assignment target evaluation is observable through an inherited accessor.
inline JsonValue cloneAssignmentCase(int mode) {
    auto prototype = JSValue::object().getPrototype();
    auto first = obj(), next = obj();
    auto s = obj({{"trace", arr()}, {"throwAt", -1}, {"current", first}, {"mode", mode}});
    JSPropertyDescriptor descriptor;
    descriptor.configurable = true;
    descriptor.enumerable = false;
    descriptor.setter = JSValue::function(
        [](const JSValue&, const Args& a, const Args& c) -> JSValue {
            event(c[0], "set-u");
            auto s = c[0];
            s.set("current", a[0]);
            s.set("first", a[0]);
            return JSValue();
        },
        {s});
    descriptor.getter = JSValue::function(
        [](const JSValue&, const Args&, const Args& c) -> JSValue {
            event(c[0], "get-u");
            if (c[0].get("mode").number() == 3)
                throw JSRangeError("lhs");
            return c[0].get("current");
        },
        {s});
    prototype.defineProperty("u", descriptor);
    const auto callback = JSValue::function(
        [](const JSValue&, const Args&, const Args& c) -> JSValue {
            event(c[0], "rhs");
            auto s = c[0];
            s.set("current", c[1]);
            return 73;
        },
        {s, next});
    JSValue value;
    if (mode == 1) {
        value = arr({obj({{"isColor", true}, {"clone", callback}})});
    } else if (mode == 2) {
        value = arr({1, 2});
        literal(value, "slice", callback);
    } else
        value = obj({{"isColor", true}, {"clone", callback}});
    auto source = obj({{"u", obj({{"value", value}})}});
    JSValue result;
    std::string error;
    try {
        result = cloneUniforms(source);
    } catch (const JSTypeError&) {
        error = "TypeError";
    } catch (const JSRangeError&) {
        error = "RangeError";
    }
    prototype.erase("u");
    Graph g;
    return JsonValue::Object{{"kind", "assignment"},
                             {"mode", mode},
                             {"error", error},
                             {"trace", s.get("trace").toJSON().value()},
                             {"first", g.encode(s.get("first"))},
                             {"next", g.encode(next)}};
}
inline JsonValue enumerationCase(int level, int mode) {
    auto prototype = obj({{"b", level == 0 ? obj({{"value", 20}}) : JSValue(20)},
                          {"inherited", level == 0 ? obj({{"value", 30}}) : JSValue(30)}});
    auto target = JSValue::object(prototype), trace = arr(), counter = obj({{"n", 0}});
    literal(target, "a", level == 0 ? obj({{"value", 1}}) : JSValue(1));
    literal(target, "b", level == 0 ? obj({{"value", 2}}) : JSValue(2));
    literal(target, "c", level == 0 ? obj({{"value", 3}}) : JSValue(3));
    auto replacement = obj({{"b", level == 0 ? obj({{"value", 80}}) : JSValue(80)},
                            {"replacement", level == 0 ? obj({{"value", 90}}) : JSValue(90)}});
    if (mode == 9) {
        JSPropertyDescriptor d;
        d.enumerable = false;
        target.defineProperty("b", d);
    }
    accessor(target, "a",
             JSValue::function(
                 [](const JSValue&, const Args&, const Args& c) -> JSValue {
                     auto target = c[0], proto = c[1], count = c[2];
                     literal(c[3], std::to_string(c[3].length()), "a");
                     const auto n = count.get("n").number();
                     count.set("n", n + 1);
                     if (n == 0) {
                         const int mode = int(c[4].number()), level = int(c[5].number());
                         auto def = [&](int x) {
                             return level == 0 ? obj({{"value", x}}) : JSValue(x);
                         };
                         if (mode == 1)
                             target.erase("c");
                         if (mode == 2)
                             target.erase("b");
                         if (mode == 3) {
                             target.erase("b");
                             proto.erase("b");
                         }
                         if (mode == 4) {
                             JSPropertyDescriptor d;
                             d.enumerable = false;
                             target.defineProperty("c", d);
                         }
                         if (mode == 5)
                             literal(target, "late", def(40));
                         if (mode == 6)
                             literal(proto, "late", def(50));
                         if (mode == 7)
                             target.setPrototype(c[6]);
                         if (mode == 8)
                             proto.erase("inherited");
                         if (mode == 9)
                             target.erase("b");
                         if (mode == 10) {
                             target.erase("c");
                             literal(target, "c", def(60));
                         }
                         if (mode == 11) {
                             target.setPrototype(nullptr);
                         }
                         if (mode == 12)
                             throw JSRangeError("getter");
                         if (mode == 13) {
                             JSPropertyDescriptor d;
                             d.enumerable = false;
                             proto.defineProperty("inherited", d);
                         }
                     }
                     return c[5].number() == 0 ? obj({{"value", 1}}) : JSValue(1);
                 },
                 {target, prototype, counter, trace, mode, level, replacement}));
    auto source = level == 0 ? target : obj({{"u", target}});
    JSValue result;
    std::string error;
    try {
        result = cloneUniforms(source);
    } catch (const JSTypeError&) {
        error = "TypeError";
    } catch (const JSRangeError&) {
        error = "RangeError";
    }
    Graph g;
    return JsonValue::Object{{"kind", "enumeration"}, {"level", level},
                             {"mode", mode},          {"trace", trace.toJSON().value()},
                             {"error", error},        {"result", g.encode(result)}};
}
