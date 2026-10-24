#include "animation/AnimationUtils.h"
#include "core/JSIterator.h"
#include "core/JSPromise.h"
#include "nodes/code/CodeNode.h"
#include "nodes/code/FunctionNode.h"
#include "nodes/core/NodeFunction.h"
#include "nodes/core/NodeFunctionInput.h"
#include "nodes/core/NodeBuilder.h"
#include <iostream>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue call(const JSValue& object, const std::string& method, const Args& args = {}) { return object.get(method).call(object, args); }
JSValue array(const Args& values = {}) { auto out = JSValue::array(); for (const auto& value : values) call(out, "push", {value}); return out; }
void data(JSValue object, const std::string& name, const JSValue& value) { JSPropertyDescriptor d; d.value = value; d.writable = d.enumerable = d.configurable = true; object.defineProperty(name, d); }
JSValue enc(const JSValue& value) { if (value.isUndefined()) return "#undefined"; if (value.isSymbol()) return "#symbol"; if (value.isCallable()) return "#function"; if (value.isObject()) return "#object"; return value; }
JSValue normal(const JSValue& value, Args seen = {}) {
    if (!value.isObject() || value.isCallable()) return enc(value);
    if (value.get("isNode").strictEquals(true)) return "@" + value.get("name").toString();
    if (value.get("_fixtureTag").toBoolean()) return "#" + value.get("_fixtureTag").toString();
    for (const auto& prior : seen) if (prior.sameValue(value)) return "#cycle"; seen.push_back(value);
    auto out = value.type() == JSValue::Type::Array ? JSValue::array(value.length()) : JSValue::object();
    if (value.type() == JSValue::Type::Array) for (std::uint32_t i = 0; i < value.length(); ++i) out.set(std::to_string(i), normal(value.get(std::to_string(i)), seen));
    else for (const auto& key : value.keys()) data(out, key, normal(value.get(key), seen)); return out;
}
void event(const JSValue& state, const Args& values) { call(state.get("events"), "push", {array(values)}); }
JSValue decode(const JSValue& value, const JSValue& state = {}) {
    if (!value.isObject()) return value;
    const auto tag = value.get("tag");
    if (tag.strictEquals("undefined")) return {};
    if (tag.strictEquals("symbol")) return JSValue::symbol("fixture");
    if (tag.strictEquals("coerce")) {
        auto out = JSValue::object(); out.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args& args, const Args& c) -> JSValue {
            event(c[0], {"coerce", arg(args, 0)}); if (c[1].get("throw").toBoolean()) throw JSTypeError("coerce"); return c[1].get("value");
        }, {state, value})); return out;
    }
    return value;
}
JSValue constructor(const std::string& name) {
    if (name == "CodeNode") return nativeCodeNodeConstructor(); if (name == "FunctionNode") return nativeFunctionNodeConstructor();
    if (name == "NodeFunction") return nativeNodeFunctionConstructor(); if (name == "NodeFunctionInput") return nativeNodeFunctionInputConstructor();
    throw JSTypeError("unknown fixture constructor");
}
JSValue node(const JSValue& ctor, const std::string& name, const Args& args = {}) { auto out = ctor.construct(args); out.set("name", name); out.set("id", name == "main" ? 100 : name == "other" ? 101 : name == "include" ? 102 : name == "a" ? 103 : 104); out.set("_uuid", "uuid-" + name); return out; }
JSValue attempt(const JSValue& object, const std::string& name, const Args& args = {}) { try { return normal(call(object, name, args)); } catch (const JSTypeError&) { return "#TypeError"; } catch (const JSRangeError&) { return "#RangeError"; } }
JSValue descriptor(const JSValue& object, const std::string& key) {
    const auto d = object.getOwnPropertyDescriptor(key).value(); const auto value = d.value.value_or(JSValue{});
    return array({key, d.value ? "data" : "accessor", enc(value), d.writable.value_or(false), d.enumerable.value_or(false), d.configurable.value_or(false),
        value.isCallable() ? value.get("name") : enc(d.getter.value_or(JSValue{})), value.isCallable() ? value.get("length") : enc(d.setter.value_or(JSValue{}))});
}
JSValue metadata(const JSValue& r) {
    const auto name = r.get("class").string(); const auto ctor = constructor(name); Args args; const auto supplied = r.get("args");
    for (std::uint32_t i = 0; i < supplied.length(); ++i) args.push_back(decode(supplied.get(std::to_string(i))));
    auto n = ctor.construct(args), p = ctor.get("prototype"), out = JSValue::object(), own = array(), ctorFields = array(), protoFields = array();
    for (const auto& key : n.ownPropertyNames()) {
        const auto d = n.getOwnPropertyDescriptor(key).value(); call(own, "push", {array({key, key == "id" ? JSValue("#id") : normal(n.get(key)), d.writable.value_or(false), d.enumerable.value_or(false), d.configurable.value_or(false)})});
    }
    for (const auto& key : ctor.ownPropertyNames()) call(ctorFields, "push", {descriptor(ctor, key)});
    for (const auto& key : p.ownPropertyNames()) call(protoFields, "push", {descriptor(p, key)});
    out.set("own", own); out.set("constructor", ctorFields); out.set("prototype", protoFields);
    out.set("parent", p.getPrototype().get("constructor").get("name")); out.set("staticParent", ctor.getPrototype().get("name")); out.set("call", attempt(p, "constructor"));
    if (name == "CodeNode" || name == "FunctionNode") {
        out.set("includesDistinct", !n.get("includes").sameValue(ctor.construct().get("includes"))); out.set("type", n.get("type")); out.set("cacheable", call(n, "isCacheable"));
        out.set("includeIdentity", call(n, "getIncludes").sameValue(n.get("includes"))); auto replacement = JSValue::object();
        const auto returned = call(n, "setIncludes", {replacement}); out.set("setIncludes", returned.sameValue(n) && n.get("includes").sameValue(replacement));
    } else if (name == "NodeFunction") out.set("abstract", attempt(n, "getCode", {"function"}));
    return out;
}
JSValue fake(const JSValue&, const Args&, const Args&);
JSValue fn(const JSValue& state, const std::string& name) { return JSValue::function(fake, {state, name}); }
JSValue fake(const JSValue& self, const Args& args, const Args& captures) {
    auto s = captures[0]; const auto name = captures[1].string(); const auto r = s.get("r"); auto builder = s.get("builder"), record = s.get("record"), n = s.get("n");
    if (name == "includeReturn") { event(s, {"closeIncludes"}); auto out = JSValue::object(); out.set("done", true); return out; }
    Args trace;
    if (name == "include0" || name == "include1") trace = {"include", name == "include0" ? 0 : 1};
    else if (name == "typeMember") trace = {"getMemberType"};
    else trace = {name, self.sameValue(name == "getCode" ? s.get("parsed") : name == "parseFunction" ? s.get("parser") : builder)};
    for (const auto& value : args) trace.push_back(normal(value)); event(s, trace);
    if (name == "parseFunction") {
        const auto count = s.get("parses").toNumber() + 1; s.set("parses", count);
        if (r.get("reenterParse").toBoolean() && count == 1) event(s, {"parseNested", attempt(n, "getNodeFunction", {builder})});
    }
    if (name == "registerDeclaration") {
        const auto count = s.get("registrations").toNumber() + 1; s.set("registrations", count);
        if (r.get("reenterRegister").toBoolean() && count == 1) event(s, {"registerNested", attempt(n, "generate", {builder, "property"})});
    }
    if (r.get("throwAt").strictEquals(name)) throw JSTypeError(name);
    if (name == "getCode") return r.hasOwn("source") ? decode(r.get("source"), s) : JSValue("function-source");
    if (name == "parseFunction") return r.hasOwn("parseResult") ? decode(r.get("parseResult"), s) : s.get("parsed");
    if (name == "getDataFromNode") return s.get("data");
    if (name == "getCodeFromNode") return record;
    if (name == "registerDeclaration") { if (r.get("rename").toBoolean()) record.set("name", "renamed"); return {}; }
    if (name == "getPropertyName") return record.get("name");
    if (name == "format") return arg(args, 0).toString() + "|" + arg(args, 1).toString() + ">" + arg(args, 2).toString();
    if (name == "getStructTypeNode") { auto out = JSValue::object(); out.set("getMemberType", fn(s, "typeMember")); return out; }
    if (name == "typeMember") return "member:" + arg(args, 1).toString();
    return {};
}
void observe(const JSValue& state, JSValue object, const std::string& key, const std::string& label) {
    const auto value = object.get(key); JSPropertyDescriptor d; d.configurable = d.enumerable = true;
    d.getter = JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue {
        event(c[0], {"get", c[3], self.sameValue(c[1])}); if (c[0].get("r").get("throwAt").strictEquals(c[3])) throw JSTypeError("getter"); return c[2];
    }, {state, object, value, label}); object.defineProperty(key, d);
}
JSValue protocol(const JSValue& r) {
    auto s = JSValue::object(); s.set("r", r); s.set("events", array()); s.set("parses", 0); s.set("registrations", 0);
    auto n = node(constructor(r.get("class").string()), "main", {"SOURCE", array(), "glsl"}), dataObject = JSValue::object(), record = JSValue::object();
    s.set("n", n); s.set("data", dataObject); s.set("record", record); record.set("name", "automatic"); record.set("type", "float"); record.set("code", "");
    auto inputs = array({nativeNodeFunctionInputConstructor().construct({"float", "value"})});
    auto parsed = nativeNodeFunctionConstructor().construct({"float", inputs, decode(r.get("name"), s), "highp"}); s.set("parsed", parsed); parsed.set("getCode", fn(s, "getCode"));
    auto parser = JSValue::object(); s.set("parser", parser); parser.set("parseFunction", fn(s, "parseFunction"));
    auto builder = JSValue::object(); s.set("builder", builder); builder.set("_fixtureTag", "builder"); builder.set("parser", parser);
    for (const auto* name : {"getDataFromNode", "getCodeFromNode", "registerDeclaration", "getPropertyName", "format", "getStructTypeNode"}) builder.set(name, fn(s, name));
    if (r.hasOwn("cached")) dataObject.set("nodeFunction", decode(r.get("cached"), s));
    if (r.hasOwn("registered")) dataObject.set("declarationRegistered", decode(r.get("registered"), s));
    auto includes = array(); for (const auto* name : {"include0", "include1"}) { auto include = JSValue::object(); include.set("build", fn(s, name)); call(includes, "push", {include}); }
    if (r.get("iterable").toBoolean()) {
        auto iterable = JSValue::object(); iterable.set(JSValue::wellKnownSymbol("iterator"), JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
            auto iterator = JSValue::object(), local = JSValue::object(); local.set("index", 0);
            iterator.set("next", JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
                auto local = c[1]; const auto index = local.get("index"); local.set("index", index.toNumber() + 1); auto out = JSValue::object();
                out.set("value", c[0].get(index)); out.set("done", index.toNumber() >= c[0].length()); return out;
            }, {c[1], local})); iterator.set("return", fn(c[0], "includeReturn")); return iterator;
        }, {s, includes})); n.set("includes", iterable);
    } else n.set("includes", includes);
    if (r.get("observed").toBoolean()) {
        for (const auto* key : {"getDataFromNode", "getCodeFromNode", "registerDeclaration", "getPropertyName", "format", "parser"}) observe(s, builder, key, std::string("builder.") + key);
        observe(s, parser, "parseFunction", "parser.parseFunction");
        for (const auto* key : {"getIncludes", "getNodeType", "getNodeFunction", "code"}) observe(s, n, key, std::string("node.") + key);
    }
    auto results = array(); for (int i = 0; i < 2; ++i) call(results, "push", {attempt(n, r.get("operation").string(), {builder, decode(r.get("output"), s)})});
    auto out = JSValue::object(); out.set("results", results); out.set("events", s.get("events")); out.set("data", normal(dataObject)); out.set("code", normal(record));
    out.set("parses", s.get("parses")); out.set("registrations", s.get("registrations")); return out;
}
JSValue serialization(const JSValue& r) {
    auto n = node(constructor(r.get("class").string()), "main", {"SOURCE", array(), "wgsl"}), a = node(nativeCodeNodeConstructor(), "a", {"A"}), b = node(nativeCodeNodeConstructor(), "b", {"B"});
    n.set("includes", array({a, b})); auto dataObject = JSValue::object(), meta = JSValue::object();
    for (const auto* key : {"nodes", "textures", "images"}) meta.set(key, JSValue::object()); dataObject.set("meta", meta);
    if (r.get("operation").strictEquals("deserialize")) {
        auto inputs = JSValue::object(), nodes = JSValue::object(); inputs.set("includes", array({"uuid-b", "uuid-a"})); nodes.set("uuid-a", a); nodes.set("uuid-b", b);
        dataObject.set("inputNodes", inputs); meta.set("nodes", nodes); dataObject.set("code", "RESTORED"); dataObject.set("language", "glsl");
    }
    const auto result = attempt(n, r.get("operation").string(), {dataObject}); auto out = JSValue::object(); out.set("result", result); out.set("data", normal(dataObject)); out.set("fields", array({normal(n.get("code")), normal(n.get("language")), normal(n.get("includes"))})); return out;
}
JSValue superMethod(const JSValue& r) {
    auto ctor = constructor(r.get("class").string()), n = node(ctor, "main", {"SOURCE"}), home = ctor.get("prototype"), saved = home.getPrototype(), events = array();
    auto replacement = JSValue::object(saved), s = JSValue::object(); s.set("r", r); s.set("n", n); s.set("events", events);
    JSPropertyDescriptor d; d.getter = JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue {
        event(c[0], {"superGet", self.sameValue(c[0].get("n"))});
        return JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            event(c[0], {"superCall", self.sameValue(c[0].get("n")), args.size()}); if (c[0].get("r").get("throw").toBoolean()) throw JSTypeError("super"); return {};
        }, c);
    }, {s}); replacement.defineProperty(r.get("operation"), d); home.setPrototype(replacement); JSValue result;
    try {
        if (r.get("class").strictEquals("CodeNode")) result = attempt(n, r.get("operation").string(), {JSValue::object()});
        else {
            n.set("getNodeFunction", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue {
                auto parsed = JSValue::object(); parsed.set("name", ""); parsed.set("type", "float"); parsed.set("getCode", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return "source"; })); return parsed;
            }));
            auto builder = JSValue::object(), code = JSValue::object(); builder.set("getCodeFromNode", JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { return c[0]; }, {code}));
            builder.set("getPropertyName", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return "property"; })); result = attempt(n, "generate", {builder, "property"});
        }
    } catch (...) { home.setPrototype(saved); throw; }
    home.setPrototype(saved); auto out = JSValue::object(); out.set("result", result); out.set("events", events); return out;
}
JSValue integration(const JSValue& r) {
    auto events = array(), parser = JSValue::object(), renderer = JSValue::object(), debug = JSValue::object(), diagnostics = JSValue::object();
    parser.set("parseFunction", JSValue::function([](const JSValue&, const Args& args, const Args& c) -> JSValue {
        call(c[0], "push", {array({"parse", arg(args, 0)})}); auto f = nativeNodeFunctionConstructor().construct({"float", array(), c[1].get("name")});
        f.set("getCode", JSValue::function([](const JSValue&, const Args& args, const Args&) -> JSValue { return "float " + arg(args, 0).toString() + "(){ return 1.0; }"; })); return f;
    }, {events, r}));
    diagnostics.set("keywords", r.get("reserved")); debug.set("diagnostics", diagnostics); renderer.set("debug", debug); renderer.set("backend", JSValue::object());
    auto builder = nativeNodeBuilderConstructor().construct({nullptr, renderer, parser}); builder.set("shaderStage", r.get("stage"));
    builder.set("isReservedKeyword", JSValue::function([](const JSValue&, const Args& args, const Args& c) -> JSValue { return arg(args, 0).strictEquals(c[0].get("name")); }, {r}));
    auto include = node(nativeCodeNodeConstructor(), "include", {"float included;"});
    auto nodes = array({node(nativeFunctionNodeConstructor(), "main", {"FUNCTION-A", array({include}), "glsl"}), node(nativeFunctionNodeConstructor(), "other", {"FUNCTION-B", array({include}), "glsl"})});
    auto results = array();
    for (const auto* stage : {"setup", "analyze", "generate"}) { call(builder, "setBuildStage", {stage}); for (std::uint32_t i = 0; i < nodes.length(); ++i) call(results, "push", {attempt(nodes.get(std::to_string(i)), "build", {builder, r.get("output")})}); }
    auto out = JSValue::object(); out.set("results", results); out.set("events", events); out.set("codes", normal(builder.get("codes")));
    const auto declarations = builder.get("declarations").get(r.get("stage")); auto keys = array(); if (declarations.toBoolean()) for (const auto& key : declarations.keys()) call(keys, "push", {key});
    out.set("declarations", keys); out.set("source", attempt(builder, "getCodes", {r.get("stage")})); out.set("chaining", builder.get("chaining").get("length")); out.set("buildStage", builder.get("buildStage")); return out;
}
JSValue gc() {
    auto keeper = JSValue::object(), result = array(); JSValue live; std::vector<JSWeakValue> weak;
    {
        auto code = nativeCodeNodeConstructor().construct(), fn = nativeFunctionNodeConstructor().construct(), input = nativeNodeFunctionInputConstructor().construct({"float", "v"}), parsed = nativeNodeFunctionConstructor().construct({"float", array({input})});
        code.set("includes", array({fn})); fn.set("includes", array({code})); fn.set("parsed", parsed); input.set("owner", code); live = code;
        for (const auto& value : {code, fn, parsed, input}) weak.emplace_back(value);
    }
    const auto snapshot = [&]() { auto values = array(); for (const auto& value : weak) call(values, "push", {!value.expired()}); call(result, "push", {values}); };
    keeper.collectGarbage(); snapshot(); live = {}; keeper.collectGarbage(); snapshot(); return result;
}
JSValue run(const JSValue& r) { const auto kind = r.get("kind"); if (kind.strictEquals("metadata")) return metadata(r); if (kind.strictEquals("protocol")) return protocol(r); if (kind.strictEquals("serialization")) return serialization(r); if (kind.strictEquals("super")) return superMethod(r); if (kind.strictEquals("integration")) return integration(r); throw JSTypeError("unknown function scenario"); }
}
int main(int argc, char** argv) {
    try {
        if (argc > 1 && std::string(argv[1]) == "gc") { std::cout << gc().toJSON()->dump(); return 0; }
        const std::string input(std::istreambuf_iterator<char>(std::cin), {}); const auto requests = Animation_detail::parseJSON(input); auto out = array();
        for (std::uint32_t i = 0; i < requests.length(); ++i) call(out, "push", {run(requests.get(std::to_string(i)))}); std::cout << out.toJSON()->dump();
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
