#pragma once
#include "core/JSValue.h"
#include "renderers/webgl/WebGLShader.h"
#include <cmath>
namespace ShaderFixture {
using Args = std::vector<JSValue>;
void data(JSValue o, const std::string& k, const JSValue& v) {
    JSPropertyDescriptor d;
    d.value = v;
    d.writable = d.enumerable = d.configurable = true;
    o.defineProperty(k, d);
}
JSValue object() {
    return JSValue::object();
}
void record(JSValue state, const JSValue& label) {
    auto log = state.get("log");
    data(log, std::to_string(log.length()), label);
    if (log.length() == state.get("throwAt").number())
        throw JSRangeError("audit");
}
JsonValue run(int variant, int construction, int point) {
    auto state = object(), gl = object(), type = object(), source = object();
    data(state, "log", JSValue::array());
    data(state, "throwAt", point);
    data(state, "variant", variant);
    JSValue shader = object();
    if (variant == 1)
        shader = nullptr;
    if (variant == 2)
        shader = 17;
    if (variant == 3)
        shader = {};
    if (variant == 4)
        shader = "handle";
    if (variant == 5)
        shader = JSValue::array();
    if (variant == 6)
        shader = JSValue::function(
            [](const JSValue&, const Args&, const Args&) -> JSValue { return {}; });
    auto replacement = JSValue::function(
        [](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            record(c[0], "replacement:" + c[1].toString());
            return {};
        },
        {state, "next"});
    for (const auto* name : {"createShader", "shaderSource", "compileShader"}) {
        auto method = JSValue::function(
            [](const JSValue& self, const Args& args, const Args& c) -> JSValue {
                auto s = c[0], gl = c[1];
                const auto name = c[2].string();
                record(s, "call:" + name);
                record(s, self.sameValue(gl));
                record(s, args.size());
                const int variant = int(s.get("variant").number());
                if (name == "createShader") {
                    record(s, args[0].sameValue(c[4]));
                    if (variant == 11)
                        data(gl, "shaderSource", c[7]);
                    if (variant == 13) {
                        auto nested = object();
                        for (const auto* k : {"createShader", "shaderSource", "compileShader"})
                            data(nested, k,
                                 JSValue::function(
                                     [](const JSValue&, const Args&, const Args& c) -> JSValue {
                                         record(c[0], "nested:" + c[1].toString());
                                         return 7;
                                     },
                                     {s, k}));
                        WebGLShader(nested, 2, "nested");
                    }
                    return c[3];
                }
                record(s, args[0].sameValue(c[3]));
                if (name == "shaderSource") {
                    record(s, args[1].sameValue(c[5]));
                    if (variant == 12)
                        data(gl, "compileShader", c[7]);
                }
                return {};
            },
            {state, gl, name, shader, type, source, variant, replacement});
        JSPropertyDescriptor d;
        d.enumerable = d.configurable = true;
        d.getter = JSValue::function(
            [](const JSValue&, const Args&, const Args& c) -> JSValue {
                record(c[0], "get:" + c[1].toString());
                const int v = int(c[0].get("variant").number());
                if ((v == 7 && c[1].strictEquals("createShader")) ||
                    (v == 8 && c[1].strictEquals("shaderSource")) ||
                    (v == 9 && c[1].strictEquals("compileShader")))
                    return 17;
                if (v == 10 && c[1].strictEquals("createShader"))
                    data(c[3], "shaderSource", c[4]);
                return c[2];
            },
            {state, name, method, gl, replacement});
        gl.defineProperty(name, d);
    }
    const auto fn = nativeWebGLShaderConstructor();
    auto proto = object();
    auto other = JSValue::constructor(
        [](const JSValue&, const Args&, const Args&) -> JSValue {
            throw JSRangeError("unexpected newTarget body");
        },
        {}, "Other", 0, JSValue::ConstructorKind::Callable);
    if (construction == 2 || construction == 4)
        other.set("prototype", proto);
    if (construction == 3)
        other.set("prototype", nullptr);
    JSValue result;
    std::string error;
    try {
        if (construction == 0)
            result = WebGLShader(gl, type, source);
        else if (construction == 1)
            result = fn.construct({gl, type, source});
        else
            result = fn.construct({gl, type, source}, other);
    } catch (const JSTypeError&) {
        error = "TypeError";
    } catch (const JSRangeError&) {
        error = "RangeError";
    }
    const auto pd = fn.getOwnPropertyDescriptor("prototype"),
               nd = fn.getOwnPropertyDescriptor("name"), ld = fn.getOwnPropertyDescriptor("length");
    return JsonValue::Object{
        {"kind", "shader"},
        {"variant", variant},
        {"construction", construction},
        {"point", point},
        {"error", error},
        {"log", state.get("log").toJSON().value()},
        {"sameShader", result.sameValue(shader)},
        {"object", result.isObject()},
        {"proto",
         result.isObject()
             ? (result.getPrototype().sameValue(fn.get("prototype"))                ? "fn"
                : result.getPrototype().sameValue(proto)                            ? "other"
                : result.getPrototype().sameValue(JSValue::object().getPrototype()) ? "object"
                                                                                    : "other-proto")
             : "primitive"},
        {"name", fn.get("name").toJSON().value()},
        {"length", fn.get("length").number()},
        {"prototypeDescriptor",
         JsonValue::Array{pd->writable.value(), pd->enumerable.value(), pd->configurable.value()}},
        {"nameDescriptor",
         JsonValue::Array{nd->writable.value(), nd->enumerable.value(), nd->configurable.value()}},
        {"lengthDescriptor",
         JsonValue::Array{ld->writable.value(), ld->enumerable.value(), ld->configurable.value()}},
        {"constructor", fn.get("prototype").get("constructor").sameValue(fn)}};
}
} // namespace ShaderFixture
