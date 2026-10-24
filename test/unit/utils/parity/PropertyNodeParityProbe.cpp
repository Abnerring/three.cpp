#include "animation/AnimationUtils.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/core/ParameterNode.h"
#include "nodes/tsl/TSLCore.h"
#include "core/NativeArray.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue call(const JSValue& object, const std::string& key, const Args& args = {}) { return object.get(key).call(object, args); }
JSValue array(const Args& args = {}) { auto out = JSValue::array(); for (const auto& value : args) call(out, "push", {value}); return out; }
JSValue enc(const JSValue& value) {
    if (value.isUndefined()) return "#undefined"; if (value.isSymbol()) return "#symbol"; if (value.isCallable()) return "#function"; if (value.isObject()) return "#object";
    if (value.type() == JSValue::Type::Number) { const auto number = value.number(); if (std::isnan(number)) return "#NaN"; if (std::isinf(number)) return number > 0 ? "#Infinity" : "#-Infinity"; if (number == 0 && std::signbit(number)) return "#-0"; } return value;
}
JSValue normal(const JSValue& value) {
    if (!value.isObject() || value.isCallable()) return enc(value);
    auto out = JSValue::object();
    if (value.get("isNode").strictEquals(true)) {
        if (value.get("isConstNode").toBoolean()) { out.set("const", true); out.set("type", value.get("nodeType")); out.set("value", enc(value.get("value"))); }
        else { out.set("node", true); out.set("type", value.get("type")); } return out;
    }
    if (value.get("_fixtureTag").toBoolean()) return "#" + value.get("_fixtureTag").toString();
    if (value.type() == JSValue::Type::Array) { out = array(); for (std::uint32_t i = 0; i < value.length(); ++i) call(out, "push", {normal(value.get(std::to_string(i)))}); }
    else for (const auto& key : value.keys()) out.set(key, normal(value.get(key))); return out;
}
void event(const JSValue& state, const Args& values) { call(state.get("events"), "push", {array(values)}); }
JSValue decode(const JSValue& value, const JSValue& state = {}) {
    if (!value.isObject()) return value;
    if (value.get("tag").strictEquals("undefined")) return {};
    if (value.get("tag").strictEquals("symbol")) return JSValue::symbol("fixture");
    if (value.get("tag").strictEquals("coerce")) {
        auto out = JSValue::object(); out.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args& args, const Args& c) -> JSValue {
            event(c[0], {"coerce", arg(args, 0)}); if (c[1].get("throw").toBoolean()) throw JSTypeError("coerce"); return c[1].get("value");
        }, {state, value})); return out;
    }
    return value;
}
JSValue attempt(const JSValue& object, const std::string& key, const Args& args = {}) { try { return normal(call(object, key, args)); } catch (const JSTypeError&) { return "#TypeError"; } catch (const JSRangeError&) { return "#RangeError"; } }
JSValue ctor(const JSValue& r) { return r.get("class").strictEquals("ParameterNode") ? nativeParameterNodeConstructor() : nativePropertyNodeConstructor(); }
JSValue metadata(const JSValue& r) {
    const auto constructor = ctor(r), supplied = r.get("args"); Args args; for (std::uint32_t i = 0; i < supplied.length(); ++i) args.push_back(decode(supplied.get(std::to_string(i))));
    auto n = constructor.construct(args), p = constructor.get("prototype"), out = JSValue::object(), own = array(), methods = array();
    for (const auto& key : n.ownPropertyNames()) { const auto d = n.getOwnPropertyDescriptor(key).value(); call(own, "push", {array({key, key == "id" ? JSValue("#id") : normal(n.get(key)), d.writable.value_or(false), d.enumerable.value_or(false), d.configurable.value_or(false)})}); }
    for (const auto& key : p.ownPropertyNames()) { const auto d = p.getOwnPropertyDescriptor(key).value(); const auto value = d.value.value(); call(methods, "push", {array({key, enc(value), value.get("name"), value.get("length"), d.writable.value_or(false), d.enumerable.value_or(false), d.configurable.value_or(false)})}); }
    out.set("own", own); out.set("methods", methods); out.set("parent", p.getPrototype().get("constructor").get("name")); out.set("staticParent", constructor.getPrototype().get("name"));
    out.set("constructor", array({constructor.get("name"), constructor.get("length")})); out.set("type", n.get("type")); out.set("abstractCall", attempt(p, "constructor")); return out;
}
JSValue fake(const JSValue&, const Args&, const Args&);
JSValue fn(const JSValue& state, const std::string& name) { return JSValue::function(fake, {state, name}); }
JSValue fake(const JSValue&, const Args& args, const Args& captures) {
    auto s = captures[0]; const auto name = captures[1].string(); const auto r = s.get("r"); Args trace{name}; for (const auto& value : args) trace.push_back(normal(value)); event(s, trace);
    if (r.get("throwAt").strictEquals(name)) throw JSTypeError(name);
    if (name == "data") return s.get("data"); if (name == "properties") return s.get("properties"); if (name == "output") return "vec4";
    if (name == "variable" || name == "varying") return s.get("variable"); if (name == "write") return decode(r.get("write"), s);
    if (name == "property") return s.get("variable").get("name"); if (name == "placeholder") return "snippet"; if (name == "member") return "vec2";
    if (name == "struct") return r.get("structure").isNull() ? JSValue(nullptr) : r.get("structure").strictEquals("undefined") ? JSValue{} : s.get("structure"); return {};
}
void observe(const JSValue& s, JSValue object, const std::string& key, const std::string& name) {
    const auto value = object.get(key); JSPropertyDescriptor d; d.configurable = true;
    d.getter = JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue {
        event(c[0], {"get", c[3], self.sameValue(c[1])}); if (c[0].get("r").get("throwAt").strictEquals(c[3])) throw JSTypeError("getter"); return c[2];
    }, {s, object, value, name}); object.defineProperty(key, d);
}
JSValue protocol(const JSValue& r) {
    auto s = JSValue::object(); s.set("events", array()); s.set("r", r);
    auto n = ctor(r).construct({r.get("nodeType"), decode(r.get("name"), s), decode(r.get("varying"), s)});
    n.set("id", decode(r.get("id").isNull() || r.get("id").isUndefined() ? JSValue(123) : r.get("id"), s));
    auto variable = JSValue::object(); variable.set("name", "variable"); variable.set("needsInterpolation", false); s.set("variable", variable);
    s.set("data", JSValue::object()); s.set("properties", JSValue::object()); auto structure = JSValue::object(); structure.set("getMemberType", fn(s, "member")); s.set("structure", structure);
    auto builder = JSValue::object(); builder.set("_fixtureTag", "builder");
    for (const auto& [method, name] : std::initializer_list<std::pair<const char*, const char*>>{{"getDataFromNode", "data"}, {"getNodeProperties", "properties"}, {"getOutputType", "output"},
        {"getVarFromNode", "variable"}, {"getVaryingFromNode", "varying"}, {"hasWriteUsage", "write"}, {"getPropertyName", "property"}, {"addLineFlowCode", "line"}, {"getStructTypeNode", "struct"}}) builder.set(method, fn(s, name));
    if (r.get("placeholder").toBoolean()) { auto placeholder = JSValue::object(); placeholder.set("build", fn(s, "placeholder")); n.set("placeholderNode", placeholder); }
    if (r.get("observed").toBoolean()) {
        for (const auto* key : {"getVarFromNode", "getVaryingFromNode", "getPropertyName", "addLineFlowCode", "getStructTypeNode", "hasWriteUsage"}) observe(s, builder, key, std::string("builder.") + key);
        for (const auto* key : {"getNodeType", "placeholderNode", "name", "varying"}) observe(s, n, key, std::string("node.") + key);
    }
    const auto result = attempt(n, r.get("operation").string(), {builder, decode(r.get("member"), s)}); auto out = JSValue::object();
    out.set("result", result); out.set("events", s.get("events")); out.set("variable", normal(variable)); out.set("data", normal(s.get("data"))); return out;
}
JSValue superMethod(const JSValue& r) {
    auto home = nativePropertyNodeConstructor().get("prototype"), prior = home.getPrototype(), n = nativePropertyNodeConstructor().construct({"float", nullptr}), s = JSValue::object(); s.set("n", n); s.set("r", r); s.set("events", array());
    auto replacement = JSValue::object(prior); JSPropertyDescriptor d;
    d.getter = JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue {
        event(c[0], {"get", self.sameValue(c[0].get("n"))}); return JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            event(c[0], {"call", self.sameValue(c[0].get("n")), args.size()}); if (c[0].get("r").get("throw").toBoolean()) throw JSTypeError("super"); return c[0].get("r").get("value");
        }, c);
    }, {s}); replacement.defineProperty(r.get("operation"), d); home.setPrototype(replacement); JSValue result;
    try { auto builder = JSValue::object(); builder.set("getOutputType", JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { event(c[0], {"output"}); return "vec4"; }, {s})); result = attempt(n, r.get("operation").string(), {builder}); }
    catch (...) { home.setPrototype(prior); throw; } home.setPrototype(prior); auto out = JSValue::object(); out.set("result", result); out.set("events", s.get("events")); return out;
}
JSValue tsl(const JSValue& r) {
    auto s = JSValue::object(); s.set("events", array()); auto value = decode(r.get("value"), s); const auto supplied = r.get("value");
    if (supplied.isObject()) {
        const auto tag = supplied.get("tag");
        if (tag.strictEquals("negativeZero")) value = -0.0;
        if (tag.strictEquals("nan")) value = std::numeric_limits<double>::quiet_NaN();
        if (tag.strictEquals("infinity")) value = std::numeric_limits<double>::infinity();
        if (tag.strictEquals("function")) { value = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return {}; }); value.set("isFn", true); }
        if (tag.strictEquals("buffer")) value = nativeArrayBufferConstructor().construct({4});
        if (tag.strictEquals("node")) value = nativePropertyNodeConstructor().construct({"float"});
        if (tag.strictEquals("flags")) {
            value = JSValue::object();
            for (const auto* flag : {"isNode", "isVector2", "isVector3", "isVector4", "isMatrix2", "isMatrix3", "isMatrix4", "isColor"}) {
                JSPropertyDescriptor descriptor; descriptor.getter = JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue {
                    event(c[0], {"get", c[3], self.strictEquals(c[1])}); if (c[2].get("throwAt").strictEquals(c[3])) throw JSTypeError("flag"); return c[2].get("value").get("flag").strictEquals(c[3]);
                }, {s, value, r, flag}); value.defineProperty(flag, descriptor);
            }
        }
        if (tag.strictEquals("typeFields")) {
            value = JSValue::object();
            for (const auto* field : {"nodeType", "convertTo"}) {
                JSPropertyDescriptor descriptor; descriptor.getter = JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue {
                    event(c[0], {"get", c[3], self.strictEquals(c[1])}); if (c[2].get("throwAt").strictEquals(c[3])) throw JSTypeError("field"); return decode(c[2].get("value").get(c[3]), c[0]);
                }, {s, value, r, field}); value.defineProperty(field, descriptor);
            }
        }
    }
    auto results = array(), raw = array();
    for (int i = 0; i < 2; ++i) {
        try {
            const auto result = r.get("operation").strictEquals("getConstNodeType") ? call(nativeTSLCore(), "getConstNodeType", {value}) : call(nativeTSLCore(), "nodeObject", {value, decode(r.get("altType"), s)});
            call(raw, "push", {result}); auto item = JSValue::object();
            if (!result.isNull() && !result.isUndefined() && result.get("isConstNode").strictEquals(true)) {
                item.set("constant", true); item.set("nodeType", enc(result.get("nodeType"))); item.set("value", normal(result.get("value"))); item.set("valueSame", result.get("value").strictEquals(value)); item.set("isWeak", result.get("isWeak"));
            } else { item.set("sameValue", result.sameValue(value)); item.set("value", enc(result)); }
            call(results, "push", {item});
        } catch (const JSTypeError&) { call(results, "push", {"#TypeError"}); call(raw, "push", {JSValue{}}); }
        catch (const JSRangeError&) { call(results, "push", {"#RangeError"}); call(raw, "push", {JSValue{}}); }
    }
    auto out = JSValue::object(); out.set("results", results); out.set("sameResult", raw.get("0").sameValue(raw.get("1"))); out.set("events", s.get("events")); return out;
}
JSValue tslIds(const JSValue& r) {
    auto result = array(); const auto values = r.get("values");
    for (std::uint32_t i = 0; i < values.length(); ++i) {
        const auto id = call(nativeTSLCore(), "nodeObject", {values.get(std::to_string(i))}).get("id");
        const auto base = call(nativeTSLCore(), "nodeObject", {r.get("base")}).get("id"); call(result, "push", {id.toNumber() - base.toNumber()});
    } return result;
}
JSValue run(const JSValue& r) { return r.get("kind").strictEquals("metadata") ? metadata(r) : r.get("kind").strictEquals("super") ? superMethod(r) : r.get("kind").strictEquals("tsl") ? tsl(r) : r.get("kind").strictEquals("tslIds") ? tslIds(r) : protocol(r); }
}
int main() {
    try { const std::string input(std::istreambuf_iterator<char>(std::cin), {}); const auto requests = Animation_detail::parseJSON(input); auto out = array(); for (std::uint32_t i = 0; i < requests.length(); ++i) call(out, "push", {run(requests.get(std::to_string(i)))}); std::cout << out.toJSON()->dump(); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
