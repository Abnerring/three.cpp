#include "animation/AnimationUtils.h"
#include "nodes/core/NodeParser.h"
#include "nodes/parsers/GLSLNodeParser.h"
#include "nodes/parsers/GLSLNodeFunction.h"
#include "renderers/webgpu/nodes/WGSLNodeParser.h"
#include "renderers/webgpu/nodes/WGSLNodeFunction.h"
#include "nodes/code/FunctionNode.h"
#include "core/JSRegExp.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue call(const JSValue& self, const std::string& name, const Args& args = {}) { return self.get(name).call(self, args); }
JSValue array(const Args& values = {}) { auto out = JSValue::array(); for (const auto& value : values) call(out, "push", {value}); return out; }
void event(const JSValue& events, const Args& values) { call(events, "push", {array(values)}); }
void data(JSValue self, const std::string& key, const JSValue& value) { JSPropertyDescriptor d; d.value = value; d.writable = d.enumerable = d.configurable = true; self.defineProperty(key, d); }
JSValue enc(const JSValue& value) {
    if (value.type() == JSValue::Type::Number) {
        const auto n = value.number();
        const char* tag = std::isnan(n) ? "NaN" : std::isinf(n) ? (n < 0 ? "-Infinity" : "Infinity") : n == 0 && std::signbit(n) ? "-0" : nullptr;
        if (tag) { auto out = JSValue::object(nullptr); data(out, "number", tag); return out; }
    }
    if (value.isUndefined()) return "#undefined"; if (value.isSymbol()) return "#symbol"; if (value.isCallable()) return "#function"; if (value.isObject()) return "#object"; return value;
}
JSValue normal(const JSValue& value) {
    if (!value.isObject() || value.isCallable()) return enc(value);
    auto out = value.type() == JSValue::Type::Array ? JSValue::array(value.length()) : JSValue::object();
    if (value.type() == JSValue::Type::Array) for (std::uint32_t i = 0; i < value.length(); ++i) out.set(std::to_string(i), normal(value.get(std::to_string(i))));
    else for (const auto& key : value.keys()) data(out, key, normal(value.get(key))); return out;
}
JSValue error(const char* name, const std::string& message = "") { auto out = JSValue::object(); out.set("error", name); if (std::string(name) == "Error") out.set("message", message); return out; }
template<class F> JSValue attempt(F&& f) { try { return normal(f()); } catch (const JSTypeError&) { return error("TypeError"); } catch (const JSRangeError&) { return error("RangeError"); } catch (const std::runtime_error& e) { return error("Error", e.what()); } }
JSValue decode(const JSValue& value, const JSValue& events, const std::string& label) {
    if (!value.isObject()) return value; const auto tag = value.get("tag");
    if (tag.strictEquals("undefined")) return {};
    if (tag.strictEquals("symbol")) return JSValue::symbol("fixture");
    if (tag.strictEquals("coerce")) {
        auto out = JSValue::object(); out.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args& args, const Args& c) -> JSValue {
            event(c[0], {"coerce", c[2], arg(args, 0)}); if (c[1].get("throw").toBoolean()) throw JSTypeError("coerce"); return c[1].get("value");
        }, {events, value, label})); return out;
    }
    if (tag.strictEquals("trim")) {
        auto out = JSValue::object(); JSPropertyDescriptor d; d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
            event(c[0], {"getTrim", c[2]}); return JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue {
                event(c[0], {"trim", c[2], self.sameValue(c[3])}); return decode(c[1].get("value"), c[0], c[2].toString() + ":trim");
            }, c);
        }, {events, value, label, out}); out.defineProperty("trim", d); return out;
    }
    return value;
}
JSValue constructor(const std::string& name) {
    if (name == "NodeParser") return nativeNodeParserConstructor(); if (name == "GLSLNodeParser") return nativeGLSLNodeParserConstructor();
    if (name == "GLSLNodeFunction") return nativeGLSLNodeFunctionConstructor(); if (name == "WGSLNodeParser") return nativeWGSLNodeParserConstructor();
    if (name == "WGSLNodeFunction") return nativeWGSLNodeFunctionConstructor(); throw JSTypeError("unknown fixture class");
}
JSValue functionClass(const JSValue& r) { return constructor(r.get("lang").string() + "NodeFunction"); }
JSValue parserClass(const JSValue& r) { return constructor(r.get("lang").string() + "NodeParser"); }
JSValue valid(const JSValue& r) { return r.get("lang").strictEquals("GLSL") ? JSValue("highp vec3 fixture(const in vec3 a, out float[4] b) { return a; }") : JSValue("fn fixture(a: vec3f, b: ptr<function, f32>) -> vec3f { return a; }"); }
JSValue descriptor(const JSValue& self, const std::string& key) {
    const auto d = self.getOwnPropertyDescriptor(key).value(); const auto value = d.value.value_or(JSValue{});
    return array({key, d.value ? "data" : "accessor", enc(value), d.writable.value_or(false), d.enumerable.value_or(false), d.configurable.value_or(false),
        value.isCallable() ? value.get("name") : enc(d.getter.value_or(JSValue{})), value.isCallable() ? value.get("length") : enc(d.setter.value_or(JSValue{}))});
}
JSValue metadata(const JSValue& r) {
    const auto name = r.get("class").string(); const auto isFunction = name.ends_with("Function"); const auto ctor = constructor(name); auto language = JSValue::object(); language.set("lang", name.starts_with("GLSL") ? "GLSL" : "WGSL");
    const auto n = isFunction ? ctor.construct({valid(language)}) : ctor.construct(), proto = ctor.get("prototype"); auto out = JSValue::object(), own = array(), ctorFields = array(), protoFields = array();
    for (const auto& key : n.ownPropertyNames()) { const auto d = n.getOwnPropertyDescriptor(key).value(); call(own, "push", {array({key, normal(n.get(key)), d.writable.value_or(false), d.enumerable.value_or(false), d.configurable.value_or(false)})}); }
    for (const auto& key : ctor.ownPropertyNames()) call(ctorFields, "push", {descriptor(ctor, key)});
    for (const auto& key : proto.ownPropertyNames()) call(protoFields, "push", {descriptor(proto, key)});
    out.set("own", own); out.set("constructor", ctorFields); out.set("prototype", protoFields);
    out.set("parent", proto.getPrototype().get("constructor").get("name")); out.set("staticParent", ctor.getPrototype().get("name"));
    out.set("call", attempt([&]() { return ctor.call(); })); out.set("methodConstruct", attempt([&]() { return proto.get(isFunction ? "getCode" : "parseFunction").construct(); })); return out;
}
JSValue parse(const JSValue& r) {
    auto events = array(), source = decode(r.get("source"), events, "source");
    if (r.get("mock").toBoolean()) {
        auto object = JSValue::object(); const auto name = r.get("lang").strictEquals("GLSL") ? "trim" : "replace";
        JSPropertyDescriptor d; d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
            event(c[0], {"sourceGet", c[3]}); if (c[1].get("throwAt").strictEquals("get")) throw JSTypeError("source get");
            return JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
                auto arguments = array(); for (const auto& a : args) call(arguments, "push", {enc(a)}); event(c[0], {"sourceCall", c[3], self.sameValue(c[4]), arguments});
                if (c[1].get("throwAt").strictEquals("call")) throw JSTypeError("source call"); return c[2];
            }, c);
        }, {events, r, source, name, object}); object.defineProperty(name, d); source = object;
    }
    auto out = JSValue::object(); out.set("result", attempt([&]() {
        const auto f = r.get("parser").toBoolean() ? call(parserClass(r).construct(), "parseFunction", {source}) : functionClass(r).construct({source});
        auto result = JSValue::object(), codes = array(), prototypes = array(); result.set("fields", normal(f));
        for (const auto& args : std::vector<Args>{{}, {JSValue{}}, {"renamed"}, {nullptr}, {JSValue::symbol("name")}}) call(codes, "push", {attempt([&]() { return call(f, "getCode", args); })});
        result.set("codes", codes); result.set("functionPrototype", f.getPrototype().sameValue(functionClass(r).get("prototype")));
        const auto inputs = f.get("inputs"); for (std::uint32_t i = 0; i < inputs.length(); ++i) call(prototypes, "push", {inputs.get(std::to_string(i)).getPrototype().get("constructor").get("name")});
        result.set("inputPrototypes", prototypes); return result;
    })); out.set("events", events); return out;
}
JSValue getCode(const JSValue& r) {
    auto events = array(), f = functionClass(r).construct({valid(r)}); const auto glsl = r.get("lang").strictEquals("GLSL");
    const std::vector<const char*> keys = glsl ? std::vector<const char*>{"name", "blockCode", "type", "inputsCode", "headerCode", "precision"} : std::vector<const char*>{"name", "outputType", "inputsCode", "blockCode"};
    for (const auto* key : keys) {
        const auto values = r.get("values"), supplied = values.isObject() && values.hasOwn(key) ? values.get(key) : f.get(key); auto state = JSValue::object(); state.set("reads", 0);
        JSPropertyDescriptor d; d.configurable = d.enumerable = true; d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
            event(c[0], {"get", c[2]}); if (c[1].get("throwAt").strictEquals(c[2])) throw JSTypeError("getCode field"); auto v = c[3];
            if (v.isObject() && v.get("tag").strictEquals("sequence")) { auto state = c[4]; const auto reads = state.get("reads").toNumber(); state.set("reads", reads + 1); const auto list = v.get("values"); v = list.get(std::to_string(static_cast<unsigned>(std::min(reads, list.get("length").toNumber() - 1)))); }
            return decode(v, c[0], c[2].toString());
        }, {events, r, key, supplied, state}); f.defineProperty(key, d);
    }
    auto out = JSValue::object(); out.set("result", attempt([&]() { return r.hasOwn("rename") ? call(f, "getCode", {decode(r.get("rename"), events, "rename")}) : call(f, "getCode"); })); out.set("events", events); return out;
}
JSValue superConstructor(const JSValue& r) {
    auto ctor = r.get("parser").toBoolean() ? parserClass(r) : functionClass(r), saved = ctor.getPrototype(), events = array();
    auto replacement = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& c) -> JSValue {
        event(c[0], {"super", normal(array(args)), newTarget.sameValue(c[2])}); if (c[1].get("throw").toBoolean()) throw JSTypeError("super");
        auto self = JSValue::object(newTarget.get("prototype")); self.set("replaced", true); return self;
    }, {events, r, ctor}, "Replacement", 0);
    auto out = JSValue::object(); ctor.setPrototype(replacement);
    try { out.set("result", attempt([&]() { return ctor.construct({r.get("source").toBoolean() ? r.get("source") : valid(r), 2, 3}); })); }
    catch (...) { ctor.setPrototype(saved); throw; }
    ctor.setPrototype(saved); out.set("events", events); return out;
}
JSValue integration(const JSValue& r) {
    auto parser = parserClass(r).construct(), f = nativeFunctionNodeConstructor().construct({r.get("source")}), events = array(), cache = JSValue::object(), record = JSValue::object(), builder = JSValue::object();
    record.set("name", "generated"); record.set("code", ""); const auto original = parser.get("parseFunction");
    parser.set("parseFunction", JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue { event(c[0], {"parse", arg(args, 0)}); return c[1].call(self, args); }, {events, original}));
    builder.set("parser", parser);
    for (const auto* name : {"getDataFromNode", "getCodeFromNode", "registerDeclaration", "getPropertyName", "format"}) builder.set(name, JSValue::function([](const JSValue&, const Args& args, const Args& c) -> JSValue {
        const auto name = c[0]; if (name.strictEquals("getDataFromNode")) { event(c[1], {"data", arg(args, 0).sameValue(c[2])}); return c[3]; }
        if (name.strictEquals("getCodeFromNode")) { event(c[1], {"code", arg(args, 0).sameValue(c[2]), arg(args, 1)}); return c[4]; }
        if (name.strictEquals("registerDeclaration")) { event(c[1], {"register", arg(args, 0).sameValue(c[4]), arg(args, 0).get("name")}); if (c[5].get("rename").toBoolean()) { auto record = c[4]; record.set("name", "reservedName"); } return {}; }
        if (name.strictEquals("getPropertyName")) { event(c[1], {"property", arg(args, 0).sameValue(c[4])}); return arg(args, 0).get("name"); }
        event(c[1], {"format", arg(args, 0), arg(args, 1), arg(args, 2)}); return arg(args, 0).toString() + ":" + arg(args, 1).toString() + ":" + arg(args, 2).toString();
    }, {name, events, f, cache, record, r}));
    const auto first = call(f, "getNodeFunction", {builder}), second = call(f, "getNodeFunction", {builder}); auto out = JSValue::object();
    out.set("cached", first.sameValue(second)); out.set("fields", normal(first)); out.set("inputsSame", call(f, "getInputs", {builder}).sameValue(first.get("inputs")));
    out.set("generated", attempt([&]() { return call(f, "generate", {builder, r.get("output")}); })); out.set("code", normal(record)); out.set("registered", cache.get("declarationRegistered").strictEquals(true)); out.set("events", events); return out;
}
JSValue regexpProtocol(const JSValue& r) {
    const auto ctor = functionClass(r); auto prototype = nativeRegExpPrototype(), events = array(), state = JSValue::object(); state.set("calls", 0);
    state.set("tokens", 0); const auto saved = prototype.getOwnPropertyDescriptor("exec").value(); JSPropertyDescriptor d; d.configurable = true;
    d.getter = JSValue::function([](const JSValue& regexp, const Args&, const Args& c) -> JSValue {
        event(c[0], {"execGet", regexp.get("source"), regexp.get("lastIndex")}); auto captures = c; captures.push_back(regexp);
        return JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            const auto regexp = c[4]; event(c[0], {"execCall", self.sameValue(regexp), arg(args, 0), regexp.get("lastIndex")}); auto state = c[2]; const auto count = state.get("calls").toNumber() + 1; state.set("calls", count);
            if (c[1].get("throwCall").strictEquals(count)) throw JSTypeError("exec"); const auto pattern = regexp.get("source").toString();
            if (c[1].get("tokens").toBoolean() && pattern == "[a-z_0-9]+") { const auto index = state.get("tokens").toNumber(); state.set("tokens", index + 1); const auto tokens = c[1].get("tokens"); return index < tokens.length() ? array({decode(tokens.get(std::to_string(static_cast<unsigned>(index))), c[0], "token")}) : JSValue(nullptr); }
            auto result = c[3].call(self, args);
            if (c[1].get("observeCaptures").toBoolean() && !result.isNull() && (pattern.starts_with("^\\s*(highp") || pattern.starts_with("^[fn]") || pattern.starts_with("[a-z_0-9]") || pattern.starts_with("([a-z_0-9]"))) {
                for (std::uint32_t i = 0; i < result.length(); ++i) { const auto value = result.get(std::to_string(i)); JSPropertyDescriptor d; d.configurable = d.enumerable = true;
                    d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { event(c[0], {"capture", c[1], c[2]}); return c[3]; }, {c[0], pattern, i, value}); result.defineProperty(std::to_string(i), d);
                }
            }
            return result;
        }, captures);
    }, {events, r, state, saved.value.value()}); prototype.defineProperty("exec", d); auto out = JSValue::object();
    try { out.set("result", attempt([&]() { const auto f = ctor.construct({valid(r)}); auto result = JSValue::object(); result.set("fields", normal(f)); result.set("code", call(f, "getCode", {"renamed"}));
        if (r.get("tokens").toBoolean()) { auto counts = array(); const auto inputs = f.get("inputs"); for (std::uint32_t i = 0; i < inputs.length(); ++i) { const auto count = inputs.get(std::to_string(i)).get("count"); call(counts, "push", {count.sameValue(-0.0) ? JSValue("#-0") : normal(count)}); } result.set("counts", counts); } return result; })); }
    catch (...) { prototype.defineProperty("exec", saved); throw; }
    prototype.defineProperty("exec", saved); out.set("events", events); return out;
}
JSValue prototypeSetters(const JSValue& r) {
    const auto ctor = r.get("parser").toBoolean() ? parserClass(r) : functionClass(r);
    auto prototype = JSValue::intrinsicObjectConstructor().get("prototype"), events = array();
    std::vector<std::pair<std::string, std::optional<JSPropertyDescriptor>>> saved;
    const auto keys = r.get("keys");
    for (std::uint32_t i = 0; i < keys.length(); ++i) {
        const auto key = keys.get(std::to_string(i)).string(); saved.emplace_back(key, prototype.getOwnPropertyDescriptor(key));
        JSPropertyDescriptor d; d.configurable = true;
        d.setter = JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            event(c[0], {"set", c[1], enc(arg(args, 0))});
            if (c[2].get("throw").toBoolean()) throw JSTypeError("prototype setter");
            data(self, c[1].string(), arg(args, 0)); return {};
        }, {events, key, r}); prototype.defineProperty(key, d);
    }
    const auto restore = [&]() { for (const auto& [key, d] : saved) { if (d) prototype.defineProperty(key, *d); else prototype.erase(key); } };
    JSValue result;
    try { result = attempt([&]() { return r.get("parser").toBoolean() ? call(ctor.construct(), "parseFunction", {valid(r)}) : ctor.construct({valid(r)}); }); }
    catch (...) { restore(); throw; }
    restore(); auto out = JSValue::object(); out.set("result", result); out.set("events", events); return out;
}
void rawEvent(JSValue events, const Args& values) {
    auto record = JSValue::array(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) data(record, std::to_string(i), values[i]);
    data(events, std::to_string(events.length()), record);
}
JSValue pushProtocol(const JSValue& r) {
    const auto ctor = r.get("parser").toBoolean() ? parserClass(r) : functionClass(r);
    auto arrayPrototype = JSValue::intrinsicArrayConstructor().get("prototype"), regexpPrototype = nativeRegExpPrototype(), events = array();
    const auto savedPush = arrayPrototype.getOwnPropertyDescriptor("push").value(), savedExec = regexpPrototype.getOwnPropertyDescriptor("exec").value();
    JSPropertyDescriptor push; push.configurable = true;
    push.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
        rawEvent(c[0], {"pushGet"}); if (c[2].get("throwAt").strictEquals("push")) throw JSTypeError("push getter"); return c[1];
    }, {events, savedPush.value.value(), r}); arrayPrototype.defineProperty("push", push);
    JSPropertyDescriptor exec; exec.writable = exec.configurable = true;
    exec.value = JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
        auto result = c[1].call(self, args);
        if (!result.isNull() && self.get("source").string().starts_with("([a-z_0-9]")) {
            for (unsigned i = 1; i <= 2; ++i) {
                const auto value = result.get(std::to_string(i)); JSPropertyDescriptor d; d.configurable = d.enumerable = true;
                d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
                    rawEvent(c[0], {"capture", c[2]}); if (c[3].get("throwAt").strictEquals("capture")) throw JSTypeError("capture getter"); return c[1];
                }, {c[0], value, i, c[2]}); result.defineProperty(std::to_string(i), d);
            }
        }
        return result;
    }, {events, savedExec.value.value(), r}); regexpPrototype.defineProperty("exec", exec);
    const auto restore = [&]() { arrayPrototype.defineProperty("push", savedPush); regexpPrototype.defineProperty("exec", savedExec); };
    JSValue result;
    try { result = attempt([&]() { return r.get("parser").toBoolean() ? call(ctor.construct(), "parseFunction", {valid(r)}) : ctor.construct({valid(r)}); }); }
    catch (...) { restore(); throw; }
    restore(); auto out = JSValue::object(); out.set("result", result); out.set("events", events); return out;
}
JSValue run(const JSValue& r) { const auto kind = r.get("kind"); if (kind.strictEquals("metadata")) return metadata(r); if (kind.strictEquals("parse")) return parse(r); if (kind.strictEquals("getCode")) return getCode(r); if (kind.strictEquals("super")) return superConstructor(r); if (kind.strictEquals("integration")) return integration(r); if (kind.strictEquals("regexp")) return regexpProtocol(r); if (kind.strictEquals("prototypeSetters")) return prototypeSetters(r); if (kind.strictEquals("pushProtocol")) return pushProtocol(r); throw JSTypeError("unknown shader parser fixture"); }
}
int main() {
    try {
        const std::string input(std::istreambuf_iterator<char>(std::cin), {}); const auto requests = Animation_detail::parseJSON(input); auto out = array();
        for (std::uint32_t i = 0; i < requests.length(); ++i) call(out, "push", {run(requests.get(std::to_string(i)))}); std::cout << out.toJSON()->dump();
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
