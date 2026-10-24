#include "animation/AnimationUtils.h"
#include "core/JSIterator.h"
#include "core/JSMap.h"
#include "core/JSPromise.h"
#include "core/JSSet.h"
#include "nodes/core/Node.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& a, std::size_t i) { return i < a.size() ? a[i] : JSValue{}; }
JSValue call(const JSValue& o, const std::string& key, const Args& a = {}) { return o.get(key).call(o, a); }
JSValue array(const Args& a = {}) {
    auto out = JSValue::array();
    for (const auto& v : a) call(out, "push", {v});
    return out;
}
void data(JSValue o, const std::string& key, const JSValue& value, bool enumerable = true) {
    JSPropertyDescriptor d;
    d.value = value; d.writable = d.configurable = true; d.enumerable = enumerable;
    o.defineProperty(key, d);
}
void accessor(JSValue o, const std::string& key, const JSValue& get, const JSValue& set = {}, bool enumerable = true) {
    JSPropertyDescriptor d;
    d.getter = get; d.setter = set; d.enumerable = enumerable; d.configurable = true;
    o.defineProperty(key, d);
}
JSValue enc(const JSValue& v) {
    if (v.isUndefined()) return "#undefined";
    if (v.isSymbol()) return "#symbol";
    if (v.isCallable()) return "#function";
    if (v.isObject()) return "#object";
    if (v.type() == JSValue::Type::Number) {
        if (std::isnan(v.number())) return "#NaN";
        if (std::isinf(v.number())) return v.number() > 0 ? "#Infinity" : "#-Infinity";
        if (v.number() == 0 && std::signbit(v.number())) return "#-0";
    }
    return v;
}
JSValue normal(const JSValue& v, Args ancestors = {}) {
    if (!v.isObject() || v.isCallable()) return enc(v);
    if (v.get("isNode").strictEquals(true)) return "@" + v.get("name").toString();
    if (v.get("_fixtureTag").strictEquals("builder")) return "#builder";
    for (const auto& a : ancestors) if (a.sameValue(v)) return "#cycle";
    ancestors.push_back(v);
    if (ancestors.size() > 12) return "#depth";
    auto out = v.type() == JSValue::Type::Array ? JSValue::array(v.length()) : JSValue::object();
    if (v.type() == JSValue::Type::Array) {
        for (std::uint32_t i = 0; i < v.length(); ++i) out.set(std::to_string(i), normal(v.get(std::to_string(i)), ancestors));
    } else for (const auto& key : v.keys()) data(out, key, normal(v.get(key), ancestors));
    return out;
}
void event(const JSValue& s, const Args& a) { call(s.get("events"), "push", {array(a)}); }
JSValue decode(const JSValue& v, const JSValue& s = {}, const std::string& label = "") {
    if (!v.isObject()) return v;
    const auto tag = v.get("tag").toString();
    if (tag == "undefined") return {};
    if (tag == "nan") return std::numeric_limits<double>::quiet_NaN();
    if (tag == "symbol") return JSValue::symbol("fixture");
    auto out = JSValue::object();
    if (tag == "coerce") out.setPrimitiveConverter(JSValue::function(
        [](const JSValue&, const Args& a, const Args& c) -> JSValue {
            event(c[0], {"coerce", c[1], arg(a, 0)});
            if (c[2].get("throw").toBoolean()) throw JSTypeError("coercion");
            return decode(c[2].get("value"));
        }, {s, label, v}));
    return out;
}
JSValue node(const std::string& name = "root", int id = 100, const Args& a = {}) {
    auto out = nativeNodeConstructor().construct(a);
    out.set("name", name); out.set("id", id); out.set("_uuid", "uuid-" + name);
    return out;
}
JSValue state(const JSValue& request) {
    auto s = JSValue::object(); s.set("request", request); s.set("events", array());
    s.set("root", node()); s.set("table", JSValue::object()); s.set("counts", JSValue::object());
    s.set("generateCalls", 0); return s;
}
JSValue attempt(const JSValue& o, const std::string& key, const Args& a = {}) {
    try { return normal(call(o, key, a)); }
    catch (const JSTypeError&) { return "#TypeError"; }
    catch (const JSRangeError&) { return "#RangeError"; }
}
JSValue entry(const JSValue& s, const JSValue& n) {
    auto table = s.get("table"); const auto name = n.get("name").toString();
    if (!table.hasOwn(name)) {
        auto e = JSValue::object(); e.set("data", JSValue::object()); e.set("properties", JSValue::object()); table.set(name, e);
    }
    return table.get(name);
}
JSValue fixture(const JSValue&, const Args&, const Args&);
JSValue fn(const JSValue& s, const std::string& name, unsigned arity = 0) { return JSValue::function(fixture, {s, name}, name, arity); }
JSValue fixture(const JSValue& self, const Args& a, const Args& c) {
    auto s = c[0]; const auto name = c[1].string(); const auto r = s.get("request");
    auto builder = s.get("builder"); const auto root = s.get("root");
    Args trace{name};
    for (const auto& v : a) trace.push_back(normal(v));
    event(s, trace);
    if (r.get("throwAt").strictEquals(name)) throw JSTypeError(name);
    if (name == "getDataFromNode") return entry(s, arg(a, 0)).get("data");
    if (name == "getNodeProperties") return entry(s, arg(a, 0)).get("properties");
    if (name == "getNodeFromHash") return s.get("shared");
    if (name == "getBuildStage") return builder.get("buildStage");
    if (name == "setBuildStage") { builder.set("buildStage", arg(a, 0)); return {}; }
    if (name == "addChain") { call(s.get("chain"), "push", {arg(a, 0).get("name")}); return {}; }
    if (name == "removeChain") { call(s.get("chain"), "pop"); return {}; }
    if (name == "increaseUsage") {
        auto d = entry(s, arg(a, 0)).get("data");
        const auto count = d.get("usageCount").isUndefined() ? 1 : d.get("usageCount").toNumber() + 1;
        d.set("usageCount", count); return r.hasOwn("usageResult") ? decode(r.get("usageResult")) : JSValue(count);
    }
    if (name == "getElementType") return "element:" + arg(a, 0).toString();
    if (name == "getVectorType") return arg(a, 0);
    if (name == "isReference") return r.hasOwn("referenceResult") ? decode(r.get("referenceResult")) : JSValue(arg(a, 0).strictEquals("reference"));
    if (name == "format") return arg(a, 0).toString() + "|" + arg(a, 1).toString() + ">" + arg(a, 2).toString();
    if (name == "generateConst") return "zero<" + arg(a, 0).toString() + ">";
    if (name == "getVarFromNode") { auto v = JSValue::object(); v.set("type", arg(a, 2)); v.set("name", "cacheVar"); return v; }
    if (name == "getPropertyName") return arg(a, 0).get("name");
    if (name == "generateLetStatement" || name == "generateVarStatement")
        return (name == "generateLetStatement" ? "let<" : "var<") + arg(a, 0).toString() + "> " + arg(a, 1).toString() + "[" + arg(a, 2).toString() + "]";
    if (name == "addLineFlowCode") { auto flow = builder.get("flow"); flow.set("code", flow.get("code").toString() + arg(a, 0).toString() + "\n"); return {}; }
    if (name == "setup") {
        if (r.get("reenterSetup").toBoolean()) event(s, {"setupNested", attempt(root, "build", {builder})});
        return s.get("outputNode");
    }
    if (name == "generate") {
        const auto count = s.get("generateCalls").toNumber() + 1; s.set("generateCalls", count);
        if (r.get("reenterGenerate").toBoolean() && count == 1) event(s, {"generateNested", attempt(root, "build", {builder, "float"})});
        if (r.get("emitFlow").toBoolean()) { auto flow = builder.get("flow"); flow.set("code", flow.get("code").toString() + "emitted;"); }
        return r.hasOwn("snippet") ? decode(r.get("snippet")) : JSValue("code");
    }
    if (name == "isCacheable") return decode(r.get("cacheable"));
    if (name == "generateNodeType") return decode(r.get("typeResult"));
    if (name == "outputBuild") return "out-code";
    if (name == "beforeBuild") { event(s, {"beforeCleared", root.get("_beforeNodes").isNull()}); return {}; }
    if (name == "overrideCallback") {
        if (r.get("reenter").toBoolean()) event(s, {"sharedNested", attempt(root, "getShared", {builder})});
        return s.get("overrideNode");
    }
    if (name == "overrideNode") return r.get("falsyOverride").toBoolean() ? JSValue(false) : s.get("overrideNode");
    if (name == "boundUpdate" || name == "boundReference") {
        event(s, {"boundReceiver", self.sameValue(root), arg(a, 0).sameValue(s.get("argument"))}); return s.get("argument");
    }
    if (name == "listener") {
        event(s, {"eventReceiver", self.sameValue(root), arg(a, 0).get("target").sameValue(root), arg(a, 0).get("type")});
        s.set("lastEvent", arg(a, 0));
        if (r.get("removeListener").toBoolean()) call(root, "removeEventListener", {"dispose", s.get("listener")});
        return {};
    }
    if (name == "childBuild") return "child-code";
    return {};
}
JSValue builder(const JSValue& s) {
    auto b = JSValue::object(); data(b, "_fixtureTag", "builder", false); auto context = JSValue::object(); b.set("context", context);
    b.set("buildStage", "setup"); b.set("shaderStage", "fragment");
    auto flow = JSValue::object(); flow.set("code", ""); b.set("flow", flow); b.set("flowBlock", nullptr);
    s.get("root"); auto mutableState = s; mutableState.set("builder", b); mutableState.set("chain", array());
    for (const auto* name : {"getDataFromNode", "getNodeProperties", "getNodeFromHash", "getBuildStage", "setBuildStage", "addChain", "removeChain", "addNode", "addSequentialNode", "increaseUsage", "getElementType", "getVectorType", "isReference", "format", "generateConst", "getVarFromNode", "getPropertyName", "generateLetStatement", "generateVarStatement", "addLineFlowCode"}) b.set(name, fn(s, name));
    return b;
}
JSValue finish(const JSValue& s, const JSValue& result) {
    auto out = JSValue::object(); out.set("result", result); out.set("events", s.get("events")); return out;
}
JSValue metadata(const JSValue& r) {
    auto s = state(r); auto n = s.get("root"); const auto ctor = nativeNodeConstructor(); auto p = ctor.get("prototype");
    auto out = JSValue::object(); auto own = array(), descriptors = array(), methods = array();
    for (const auto& key : n.ownPropertyNames()) {
        call(own, "push", {key}); const auto d = n.getOwnPropertyDescriptor(key).value();
        call(descriptors, "push", {array({key, enc(n.get(key)), d.writable.value_or(false), d.enumerable.value_or(false), d.configurable.value_or(false)})});
    }
    for (const auto& key : p.ownPropertyNames()) {
        const auto d = p.getOwnPropertyDescriptor(key).value(); const auto v = p.get(key);
        call(methods, "push", {array({key, d.value.has_value() ? JSValue("data") : JSValue("accessor"),
            v.isCallable() ? v.get("name") : enc(v), enc(v.isCallable() ? v.get("length") : JSValue{}),
            d.writable.value_or(false), d.enumerable.value_or(false), d.configurable.value_or(false),
            d.getter ? d.getter->isCallable() : false, d.setter ? d.setter->isCallable() : false})});
    }
    out.set("own", own); out.set("descriptors", descriptors); out.set("methods", methods);
    out.set("staticType", ctor.get("type")); out.set("capture", ctor.get("captureStackTrace"));
    out.set("constructorName", ctor.get("name")); out.set("constructorLength", ctor.get("length"));
    out.set("callConstructor", attempt(p, "constructor"));
    auto a = nativeNodeConstructor().construct(), b = nativeNodeConstructor().construct();
    out.set("idStep", b.get("id").toNumber() - a.get("id").toNumber());
    out.set("scope", call(n, "getScope").sameValue(n)); out.set("reference", call(n, "updateReference").sameValue(n));
    out.set("hash", call(n, "getHash")); out.set("uuid", n.get("uuid"));
    return out;
}
JSValue updates(const JSValue& r) {
    auto s = state(r); auto n = s.get("root"); n.set("version", decode(r.get("version"), s, "version"));
    auto values = array(); const auto inputs = r.get("values");
    for (std::uint32_t i = 0; i < inputs.length(); ++i) {
        try { n.set("needsUpdate", decode(inputs.get(std::to_string(i)))); call(values, "push", {enc(n.get("version"))}); }
        catch (const JSTypeError&) { call(values, "push", {"#TypeError"}); }
    }
    auto out = finish(s, values); out.set("needsOwn", n.hasOwn("needsUpdate")); out.set("needsRead", enc(n.get("needsUpdate")));
    auto c = JSValue::object(); c.set("type", decode(r.get("type"))); n.set("constructor", c); out.set("type", enc(n.get("type")));
    for (const auto* key : {"updateType", "updateBeforeType", "updateAfterType", "global"}) n.set(key, decode(r.get("type")));
    out.set("accessors", array({enc(call(n, "getUpdateType")), enc(call(n, "getUpdateBeforeType")), enc(call(n, "getUpdateAfterType")), enc(call(n, "isGlobal")), call(n, "isCacheable"), call(n, "getArrayCount"), call(n, "getMemberType")}));
    return out;
}
JSValue children(const JSValue& r) {
    auto s = state(r); auto n = s.get("root"), a = node("a", 101), b = node("b", 102);
    auto list = array({b, nullptr, a}); auto dict = JSValue::object(); dict.set("first", a); dict.set("_private", b); data(dict, "hidden", b, false);
    n.set("direct", a); n.set("list", list); n.set("dictionary", dict); n.set("self", n); data(n, "hidden", b, false);
    auto wrong = JSValue::object(); wrong.set("isNode", 1); n.set("wrong", wrong);
    auto nullProto = JSValue::object(nullptr); nullProto.set("child", b); n.set("nullPrototype", nullProto);
    accessor(n, "_private", JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { event(c[0], {"privateGet"}); return c[1]; }, {s, a}));
    accessor(n, "dynamic", JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
        event(c[0], {"dynamicGet"});
        auto root = c[0].get("root"); const auto mode = c[0].get("request").get("behavior");
        if (mode.strictEquals("delete")) root.erase("tail");
        if (mode.strictEquals("throw")) throw JSTypeError("child getter");
        return c[1];
    }, {s, b}));
    n.set("tail", a);
    if (r.get("behavior").strictEquals("array-self")) call(list, "push", {n});
    auto ignores = createNativeSet(); if (r.get("ignore").toBoolean()) call(ignores, "add", {a});
    auto inheritedPrototype = JSValue::object().getPrototype();
    const auto saved = inheritedPrototype.getOwnPropertyDescriptor("nodeFixtureInherited");
    if (r.get("inherited").toBoolean()) data(inheritedPrototype, "nodeFixtureInherited", b);
    JSValue result;
    try {
        if (r.get("generator").toBoolean()) {
            auto iterator = getJSIterator(call(n, "getChildren"));
            if (r.get("lazy").toBoolean()) n.set("late", b);
            auto values = array();
            while (const auto value = stepJSIterator(iterator)) call(values, "push", {normal(*value)});
            result = values;
        } else result = attempt(n, "_getChildren", {ignores});
    } catch (const JSTypeError&) { result = "#TypeError"; }
    if (r.get("inherited").toBoolean()) { if (saved) inheritedPrototype.defineProperty("nodeFixtureInherited", *saved); else inheritedPrototype.erase("nodeFixtureInherited"); }
    auto out = finish(s, result); out.set("ignoredRoot", call(ignores, "has", {n})); return out;
}
JSValue cache(const JSValue& r) {
    auto s = state(r); auto n = s.get("root"), a = node("a", 101), b = node("b", 102);
    n.set("firstNode", a); n.set("secondNode", b); a.set("back", n);
    auto results = array();
    for (int i = 0; i < 4; ++i) {
        if (i == 1) a.set("version", 1);
        if (i == 2) n.set("needsUpdate", true);
        call(results, "push", {attempt(n, "getCacheKey", {i == 0 ? decode(r.get("force")) : JSValue(false)})});
    }
    auto out = finish(s, results); out.set("version", n.get("_cacheKeyVersion")); out.set("key", enc(n.get("_cacheKey"))); return out;
}
JSValue types(const JSValue& r) {
    auto s = state(r); auto n = s.get("root"), b = builder(s); n.set("nodeType", decode(r.get("nodeType")));
    if (r.get("custom").toBoolean()) n.set("generateNodeType", fn(s, "generateNodeType", 2));
    if (r.get("outputNode").toBoolean()) {
        auto output = node("output", 200); output.set("getNodeType", fn(s, "generateNodeType", 2)); entry(s, n).get("properties").set("outputNode", output);
    }
    auto values = array(); const auto outputs = r.get("outputs");
    for (std::uint32_t i = 0; i < outputs.length(); ++i) call(values, "push", {attempt(n, "getNodeType", {b, decode(outputs.get(std::to_string(i)))})});
    auto out = finish(s, values); out.set("element", attempt(n, "getElementType", {b})); out.set("data", normal(entry(s, n).get("data"))); return out;
}
JSValue shared(const JSValue& r) {
    auto s = state(r); auto n = s.get("root"), b = builder(s), other = node("other", 200);
    const auto mode = r.get("mode");
    if (mode.strictEquals("same")) s.set("shared", n);
    if (mode.strictEquals("other")) s.set("shared", other);
    if (mode.strictEquals("falsy")) s.set("shared", 0);
    if (mode.strictEquals("override")) {
        auto overrides = createNativeMap(); call(overrides, "set", {n, fn(s, "overrideCallback", 1)}); b.get("context").set("overrideNodes", overrides);
        other.set("overrideNode", fn(s, "overrideNode", 2)); s.set("overrideNode", other);
    }
    auto values = array(); for (int i = 0; i < 3; ++i) call(values, "push", {attempt(n, "getShared", {b})});
    auto out = finish(s, values); out.set("data", normal(entry(s, n).get("data"))); return out;
}
JSValue build(const JSValue& r) {
    auto s = state(r); auto n = s.get("root"), b = builder(s); auto d = entry(s, n).get("data"), p = entry(s, n).get("properties");
    n.set("nodeType", "float"); n.set("parents", r.get("parents").toBoolean());
    if (r.get("customSetup").toBoolean()) n.set("setup", fn(s, "setup", 1));
    if (r.get("customAnalyze").toBoolean()) n.set("analyze", fn(s, "analyze", 2));
    if (r.hasOwn("generateArity")) n.set("generate", fn(s, "generate", static_cast<unsigned>(r.get("generateArity").toNumber())));
    if (r.hasOwn("cacheable")) n.set("isCacheable", fn(s, "isCacheable", 1));
    n.set("global", r.get("global").toBoolean());
    if (r.get("outputNode").toBoolean()) {
        auto o = node("output", 200); o.set("build", fn(s, "outputBuild", 2)); s.set("outputNode", o);
        if (!r.get("customSetup").toBoolean()) p.set("outputNode", o);
    }
    if (r.get("children").toBoolean()) {
        auto child = node("child", 300); child.set("build", fn(s, "childBuild", 2)); child.set("parents", true); n.set("child", child);
    }
    if (r.get("before").toBoolean()) { auto before = node("before", 400); before.set("build", fn(s, "beforeBuild", 2)); call(n, "before", {before}); }
    if (r.get("prebuilt").toBoolean()) { auto stages = JSValue::object(); stages.set("setup", true); stages.set("analyze", true); d.set("buildStages", stages); }
    if (r.hasOwn("usage")) d.set("usageCount", decode(r.get("usage")));
    if (r.hasOwn("assign")) d.set("assign", decode(r.get("assign")));
    if (r.hasOwn("cachedSnippet")) d.set("snippet", decode(r.get("cachedSnippet")));
    if (r.hasOwn("cachedProperty")) d.set("propertyName", decode(r.get("cachedProperty")));
    if (r.get("block").toBoolean()) {
        auto old = JSValue::object(); old.set("parent", nullptr); old.set("label", "old");
        auto current = JSValue::object(); current.set("label", "current"); current.set("parent", r.get("block").strictEquals("inside") ? old : JSValue(nullptr));
        d.set("flowBlock", old); b.set("flowBlock", current);
    }
    auto results = array(); const auto steps = r.get("steps");
    for (std::uint32_t i = 0; i < steps.length(); ++i) {
        const auto step = steps.get(std::to_string(i)); b.set("buildStage", step.get("stage"));
        const auto value = attempt(n, r.get("direct").isUndefined() ? "build" : r.get("direct").string(), {b, decode(step.get("output"))});
        auto row = JSValue::object(); row.set("value", value); row.set("stage", b.get("buildStage")); row.set("data", normal(d)); row.set("properties", normal(p)); row.set("chain", normal(s.get("chain"))); call(results, "push", {row});
    }
    auto out = finish(s, results); out.set("table", normal(s.get("table"))); out.set("flow", b.get("flow").get("code")); out.set("beforeNodes", normal(n.get("_beforeNodes"))); return out;
}
JSValue bindings(const JSValue& r) {
    auto s = state(r); auto n = s.get("root"), argument = JSValue::object(); s.set("argument", argument);
    auto results = array();
    const auto mode = r.get("mode").string();
    if (mode == "dispose") {
        auto listener = fn(s, "listener", 1); s.set("listener", listener); call(n, "addEventListener", {"dispose", listener}); call(n, "addEventListener", {"dispose", listener});
        for (int i = 0; i < 2; ++i) call(results, "push", {attempt(n, "dispose")});
        call(results, "push", {enc(s.get("lastEvent").get("target")), call(n, "hasEventListener", {"dispose", listener})});
    } else {
        const auto update = fn(s, mode == "onReference" ? "boundReference" : "boundUpdate", 1);
        call(results, "push", {attempt(n, mode, mode == "onUpdate" ? Args{update, decode(r.get("updateType"))} : Args{update})});
        call(results, "push", {normal(n.get(mode == "onReference" ? "updateReference" : "update").call(JSValue::object(), {argument}))});
        call(results, "push", {enc(n.get("updateType"))});
    }
    return finish(s, results);
}
JSValue traverse(const JSValue& r) {
    auto s = state(r); auto root = s.get("root"), a = node("a", 101), b = node("b", 102), c = node("c", 103);
    root.set("left", a); root.set("right", b); a.set("child", c);
    if (r.get("duplicate").toBoolean()) root.set("again", a);
    auto callback = JSValue::function([](const JSValue& self, const Args& a, const Args& c) -> JSValue {
        auto s = c[0]; const auto name = arg(a, 0).get("name"); event(s, {"visit", name, self.isUndefined()});
        const auto request = s.get("request");
        if (request.get("throwAt").strictEquals(name)) throw JSTypeError("visit");
        if (request.get("mutate").toBoolean() && name.strictEquals("root")) { auto root = s.get("root"); root.erase("right"); root.set("late", c[1]); }
        return false;
    }, {s, c}, "visit", 1);
    return finish(s, attempt(root, "traverse", {callback}));
}
JSValue bindingOrder(const JSValue& r) {
    auto s = state(r); auto root = s.get("root"); auto callback = JSValue::object();
    s.set("callback", callback); s.set("replacement", JSValue::object());
    const auto operation = r.get("operation").string();
    accessor(callback, "bind", JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue {
        event(c[0], {"getBind", self.sameValue(c[0].get("callback"))});
        if (c[0].get("request").get("throwAt").strictEquals("bindGet")) throw JSTypeError("bind");
        return JSValue::function([](const JSValue& self, const Args& a, const Args& c) -> JSValue {
            event(c[0], {"callBind", self.sameValue(c[0].get("callback")), arg(a, 0).sameValue(c[0].get("root")), a.size()});
            if (c[0].get("request").get("throwAt").strictEquals("bindCall")) throw JSTypeError("bind");
            return c[0].get("replacement");
        }, c);
    }, {s}));
    for (const auto* name : {"updateType", "update", "updateReference"}) accessor(root, name, {}, JSValue::function(
        [](const JSValue& self, const Args& a, const Args& c) -> JSValue {
            event(c[0], {"set", c[1], self.sameValue(c[0].get("root")), arg(a, 0).sameValue(c[0].get("replacement")) ? JSValue("#replacement") : enc(arg(a, 0))});
            if (c[0].get("request").get("throwAt").strictEquals("set:" + c[1].string())) throw JSTypeError("setter");
            return {};
        }, {s, name}));
    return finish(s, attempt(root, operation, operation == "onUpdate" ? Args{callback, "custom"} : Args{callback}));
}
void observeGetter(const JSValue& s, JSValue object, const std::string& key, const std::string& label) {
    const auto value = object.get(key);
    accessor(object, key, JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue {
        event(c[0], {"get", c[1], self.sameValue(c[2])});
        if (c[0].get("request").get("throwAt").strictEquals(c[1])) throw JSTypeError("getter");
        return c[3];
    }, {s, label, object, value}));
}
JSValue evaluationOrder(const JSValue& r) {
    auto s = state(r); auto root = s.get("root"), b = builder(s); auto d = entry(s, root).get("data"), p = entry(s, root).get("properties");
    root.set("nodeType", "float");
    const auto operation = r.get("operation").string(); auto output = node("output", 200);
    output.set("build", fn(s, "outputBuild", 2)); output.set("getNodeType", fn(s, "generateNodeType", 2));
    s.get("request");
    if (operation == "generateNodeType" || operation == "generate") p.set("outputNode", output);
    if (operation == "build") {
        b.set("buildStage", "generate"); auto stages = JSValue::object(); stages.set("setup", true); stages.set("analyze", true); d.set("buildStages", stages); d.set("usageCount", 1);
        if (r.get("cached").toBoolean()) d.set("propertyName", "existing");
        root.set("generate", fn(s, "generate", 2));
    }
    for (const auto* name : {"getDataFromNode", "getNodeProperties", "getNodeFromHash", "getVectorType", "format", "getElementType"}) observeGetter(s, b, name, std::string("builder.") + name);
    for (const auto* name : {"getNodeType", "generateNodeType", "getHash", "isCacheable", "generate"}) observeGetter(s, root, name, std::string("node.") + name);
    observeGetter(s, root, "nodeType", "node.nodeType");
    observeGetter(s, p, "outputNode", "properties.outputNode");
    observeGetter(s, d, "propertyName", "data.propertyName");
    if (operation == "serialize") {
        auto child = node("child", 300); root.set("single", child);
        auto json = JSValue::object(), meta = JSValue::object(); meta.set("nodes", JSValue::object()); meta.set("textures", JSValue::object()); meta.set("images", JSValue::object()); json.set("meta", meta);
        observeGetter(s, json, "meta", "json.meta"); observeGetter(s, child, "toJSON", "child.toJSON");
        return finish(s, attempt(root, "serialize", {json}));
    }
    const auto result = attempt(root, operation, {b, "vec3"});
    return finish(s, result);
}
JSValue generator(const JSValue& r) {
    auto s = state(r); auto root = s.get("root"), a = node("a", 101), b = node("b", 102);
    auto records = array();
    for (const auto& n : {a, b}) { auto record = JSValue::object(); record.set("childNode", n); call(records, "push", {record}); }
    s.set("records", records);
    root.set("_getChildren", JSValue::function([](const JSValue& self, const Args& a, const Args& c) -> JSValue {
        auto s = c[0]; event(s, {"childrenCall", self.sameValue(s.get("root")), a.size()});
        const auto request = s.get("request");
        if (request.get("reenter").toBoolean()) event(s, {"nested", attempt(s.get("iterator"), request.get("nested").string(), {"closed"})});
        if (request.get("throwAt").strictEquals("children")) throw JSTypeError("children");
        return s.get("records");
    }, {s}, "_getChildren", 0));
    auto iterator = call(root, "getChildren"); s.set("iterator", iterator);
    auto result = array(); call(result, "push", {array({"created", s.get("events").length()})});
    if (r.get("returnFirst").toBoolean()) call(result, "push", {attempt(iterator, "return", {"early"})});
    for (int i = 0; i < 3; ++i) call(result, "push", {attempt(iterator, "next")});
    call(result, "push", {attempt(iterator, "return", {"late"})});
    return finish(s, result);
}
JSValue generatorCall(const JSValue& s, const JSValue& method, const JSValue& receiver, const Args& args = {}) {
    try { return normal(method.call(receiver, args)); }
    catch (const JSThrownValue& e) {
        auto out = JSValue::object(); out.set("thrown", normal(e.value));
        out.set("sameToken", e.value.sameValue(s.get("token"))); out.set("sameCloseToken", e.value.sameValue(s.get("closeToken"))); return out;
    }
    catch (const JSTypeError&) { return "#TypeError"; }
    catch (const JSRangeError&) { return "#RangeError"; }
}
JSValue generatorAttempt(const JSValue& s, const JSValue& iterator, const std::string& method, const Args& args = {}) {
    return generatorCall(s, iterator.get(method), iterator, args);
}
void generatorNested(const JSValue& s, const std::string& at) {
    const auto r = s.get("request");
    if (r.get("nestedAt").strictEquals(at)) event(s, {"nested", at, generatorAttempt(s, s.get("iterator"), r.get("nested").string(), {s.get("token")})});
}
JSValue protocolGenerator(const JSValue& s, const std::string& label) {
    auto root = node(label, label == "main" ? 400 : 500), inner = JSValue::object(), iterable = JSValue::object();
    auto local = JSValue::object(); local.set("index", 0); local.set("root", root); local.set("inner", inner); local.set("iterable", iterable);
    local.set("children", array({node(label + ".a", 401), node(label + ".b", 402)}));
    accessor(inner, "next", JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue {
        const auto s = c[0], local = c[2]; event(s, {"nextGet", c[1], self.sameValue(local.get("inner"))});
        if (s.get("request").get("fault").strictEquals("nextGet")) throw JSThrownValue(s.get("token"));
        return JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            const auto s = c[0], r = s.get("request"); auto local = c[2];
            event(s, {"innerNext", c[1], self.sameValue(local.get("inner")), args.size()});
            if (r.get("fault").strictEquals("nextCall")) throw JSThrownValue(s.get("token"));
            if (r.get("fault").strictEquals("nextResult")) return 5;
            const auto i = local.get("index"); local.set("index", i.toNumber() + 1); auto record = JSValue::object();
            accessor(record, "childNode", JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
                event(c[0], {"child", c[1], c[3]});
                if (c[0].get("request").get("fault").strictEquals("childGet")) throw JSThrownValue(c[0].get("token"));
                return c[2].get("children").get(c[3].toString());
            }, {s, c[1], local, i}), {}, false);
            auto result = JSValue::object();
            accessor(result, "done", JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
                event(c[0], {"done", c[1], c[3]});
                if (c[0].get("request").get("fault").strictEquals("doneGet")) throw JSThrownValue(c[0].get("token"));
                return c[3].toNumber() >= c[2].get("children").length();
            }, {s, c[1], local, i}));
            accessor(result, "value", JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
                event(c[0], {"value", c[1], c[3]});
                const auto fault = c[0].get("request").get("fault");
                if (fault.strictEquals("valueGet")) throw JSThrownValue(c[0].get("token"));
                return fault.strictEquals("childNull") ? JSValue(nullptr) : c[4];
            }, {s, c[1], local, i, record}));
            return result;
        }, c);
    }, {s, label, local}), {}, false);
    accessor(inner, "return", JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue {
        const auto s = c[0], r = s.get("request"); event(s, {"returnGet", c[1], self.sameValue(c[2].get("inner"))}); generatorNested(s, "get");
        const auto close = r.get("close");
        if (close.strictEquals("getThrow")) throw JSThrownValue(s.get("closeToken"));
        if (close.strictEquals("missing")) return {};
        if (close.strictEquals("null")) return nullptr;
        if (close.strictEquals("nonCallable")) return 5;
        return JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            const auto s = c[0]; event(s, {"innerReturn", c[1], self.sameValue(c[2].get("inner")), args.size()}); generatorNested(s, "call");
            const auto close = s.get("request").get("close");
            if (close.strictEquals("callThrow")) throw JSThrownValue(s.get("closeToken"));
            if (close.strictEquals("primitive")) return 5;
            auto out = JSValue::object(); out.set("value", "inner-return"); out.set("done", true); return out;
        }, c);
    }, {s, label, local}), {}, false);
    inner.set("throw", JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
        event(c[0], {"innerThrow", c[1], self.sameValue(c[2].get("inner")), args.size()}); auto out = JSValue::object(); out.set("done", true); return out;
    }, {s, label, local}));
    JSPropertyDescriptor iteratorDescriptor; iteratorDescriptor.configurable = true;
    iteratorDescriptor.getter = JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue {
        const auto s = c[0]; event(s, {"iteratorGet", c[1], self.sameValue(c[2].get("iterable"))});
        if (s.get("request").get("fault").strictEquals("iteratorGet")) throw JSThrownValue(s.get("token"));
        return JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            const auto s = c[0]; event(s, {"iteratorCall", c[1], self.sameValue(c[2].get("iterable")), args.size()});
            const auto fault = s.get("request").get("fault");
            if (fault.strictEquals("iteratorCall")) throw JSThrownValue(s.get("token"));
            return fault.strictEquals("iteratorResult") ? JSValue(5) : c[2].get("inner");
        }, c);
    }, {s, label, local});
    iterable.defineProperty(JSValue::wellKnownSymbol("iterator"), iteratorDescriptor);
    root.set("_getChildren", JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
        event(c[0], {"children", c[1], self.sameValue(c[2].get("root")), args.size()}); return c[2].get("iterable");
    }, {s, label, local}));
    return call(root, "getChildren");
}
JSValue generatorDescriptor(const JSValue& object, const JSValue& key) {
    const auto d = object.getOwnPropertyDescriptor(key);
    if (!d) return nullptr;
    return array({d->value ? JSValue("data") : JSValue("accessor"), enc(d->value.value_or(JSValue{})), d->writable.value_or(false),
        d->enumerable.value_or(false), d->configurable.value_or(false), enc(d->getter.value_or(JSValue{})), enc(d->setter.value_or(JSValue{}))});
}
JSValue generatorNames(const JSValue& object, bool symbols = false) {
    auto out = array();
    if (symbols) for (const auto& symbol : object.ownPropertySymbols()) call(out, "push", {symbol.symbolDescription().value_or("")});
    else for (const auto& name : object.ownPropertyNames()) call(out, "push", {name});
    return out;
}
void generatorAdvance(const JSValue& s, const JSValue& iterator, const JSValue& stage, const JSValue& result) {
    if (stage.strictEquals("suspended")) call(result, "push", {array({"prepare", generatorAttempt(s, iterator, "next")})});
    if (stage.strictEquals("closed")) call(result, "push", {array({"prepare", generatorAttempt(s, iterator, "return", {"prepared"})})});
    if (stage.strictEquals("exhausted")) for (int i = 0; i < 3; ++i) call(result, "push", {array({"prepare", generatorAttempt(s, iterator, "next")})});
}
JSValue generatorProtocol(const JSValue& r) {
    auto s = state(r), token = JSValue::object(), closeToken = JSValue::object(); token.set("token", "outer"); closeToken.set("token", "close");
    s.set("token", token); s.set("closeToken", closeToken);
    auto iterator = protocolGenerator(s, "main"), result = array(); s.set("iterator", iterator);
    const auto mode = r.get("mode");
    if (mode.strictEquals("metadata")) {
        const auto other = protocolGenerator(s, "other"), method = nativeNodeConstructor().get("prototype").get("getChildren");
        const auto p = iterator.getPrototype(), common = p.getPrototype(), symbolIterator = JSValue::wellKnownSymbol("iterator"), symbolTag = JSValue::wellKnownSymbol("toStringTag");
        auto fields = JSValue::object(); fields.set("iteratorOwn", generatorNames(iterator)); fields.set("iteratorSymbols", generatorNames(iterator, true));
        fields.set("prototypeOwn", generatorNames(p)); fields.set("prototypeSymbols", generatorNames(p, true));
        fields.set("prototypeIsMethodPrototype", p.sameValue(method.get("prototype"))); fields.set("commonOwn", generatorNames(common)); fields.set("commonSymbols", generatorNames(common, true));
        fields.set("tag", enc(iterator.get(symbolTag))); fields.set("tagDescriptor", generatorDescriptor(common, symbolTag));
        fields.set("methodOwn", generatorNames(method)); fields.set("methodPrototypeDescriptor", generatorDescriptor(method, "prototype")); fields.set("methodConstructible", method.isConstructible());
        fields.set("iteratorReturnsSelf", iterator.get(symbolIterator).call(iterator).sameValue(iterator));
        fields.set("genericIteratorReturnsReceiver", iterator.get(symbolIterator).call(token).sameValue(token)); call(result, "push", {fields});
        for (const auto* key : {"next", "return", "throw"}) {
            const auto fn = iterator.get(key); auto owner = iterator; int depth = 0;
            while (!owner.isNull() && !owner.hasOwn(key)) { owner = owner.getPrototype(); ++depth; }
            call(result, "push", {array({key, iterator.hasOwn(key), fn.sameValue(other.get(key)), depth, generatorDescriptor(owner, key), generatorNames(fn),
                fn.get("name"), fn.get("length"), fn.isConstructible(), fn.getPrototype().sameValue(nativeNodeConstructor().get("prototype").get("getHash").getPrototype())})});
        }
    } else if (mode.strictEquals("brand")) {
        generatorAdvance(s, iterator, r.get("stage"), result); const auto name = r.get("receiver"); JSValue receiver;
        if (name.strictEquals("object")) receiver = JSValue::object();
        else if (name.strictEquals("directSpoof")) receiver = JSValue::object(iterator.getPrototype());
        else if (name.strictEquals("commonSpoof")) receiver = JSValue::object(iterator.getPrototype().getPrototype());
        else if (name.strictEquals("array")) receiver = array();
        else if (name.strictEquals("node")) receiver = s.get("root");
        else if (name.strictEquals("null")) receiver = nullptr;
        else if (name.strictEquals("number")) receiver = 1;
        else if (name.strictEquals("function")) receiver = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return {}; });
        call(result, "push", {array({"borrowed", generatorCall(s, iterator.get(r.get("method")), receiver, {token})})});
        call(result, "push", {array({"original", generatorAttempt(s, iterator, "next")})});
    } else if (mode.strictEquals("borrow")) {
        auto other = protocolGenerator(s, "other"); generatorAdvance(s, iterator, r.get("stage"), result); generatorAdvance(s, other, r.get("otherStage"), result);
        call(result, "push", {array({"borrowed", generatorCall(s, iterator.get(r.get("method")), other, {token})})});
        call(result, "push", {array({"original", generatorAttempt(s, iterator, "next")})}); call(result, "push", {array({"other", generatorAttempt(s, other, "next")})});
    } else {
        generatorAdvance(s, iterator, r.get("stage"), result);
        call(result, "push", {array({"action", generatorAttempt(s, iterator, r.get("method").string(), {token})})});
        for (const auto* method : {"next", "return", "throw", "next"}) call(result, "push", {array({"after", method, generatorAttempt(s, iterator, method, {token})})});
    }
    return finish(s, result);
}
JSValue enumerationMutation(const JSValue& r) {
    auto s = state(r), dictionary = JSValue::object(), a = node("a", 101), b = node("b", 102), c = node("c", 103);
    auto prototype = JSValue::object().getPrototype(); const auto saved = prototype.getOwnPropertyDescriptor("nodeParityMutation");
    accessor(dictionary, "first", JSValue::function([](const JSValue& self, const Args&, const Args& captures) -> JSValue {
        const auto s = captures[0], mode = s.get("request").get("mode"); auto dictionary = captures[1], prototype = captures[5];
        event(s, {"getFirst", self.sameValue(dictionary)});
        if (mode.strictEquals("addPrototype")) data(prototype, "nodeParityMutation", captures[3]);
        if (mode.strictEquals("deletePrototype")) prototype.erase("nodeParityMutation");
        if (mode.strictEquals("hidePrototype") || mode.strictEquals("revealPrototype")) {
            JSPropertyDescriptor d; d.enumerable = mode.strictEquals("revealPrototype"); prototype.defineProperty("nodeParityMutation", d);
        }
        if (mode.strictEquals("replacePrototypeValue")) prototype.set("nodeParityMutation", captures[4]);
        if (mode.strictEquals("addOwn")) data(dictionary, "nodeParityMutation", captures[3]);
        if (mode.strictEquals("deleteOwn") || mode.strictEquals("deleteShadow")) dictionary.erase("nodeParityMutation");
        if (mode.strictEquals("hideOwn") || mode.strictEquals("revealOwn")) {
            JSPropertyDescriptor d; d.enumerable = mode.strictEquals("revealOwn"); dictionary.defineProperty("nodeParityMutation", d);
        }
        return captures[2];
    }, {s, dictionary, a, b, c, prototype}));
    s.get("root").set("dictionary", dictionary);
    const auto mode = r.get("mode"); JSValue result;
    const auto restore = [&]() { if (saved) prototype.defineProperty("nodeParityMutation", *saved); else prototype.erase("nodeParityMutation"); };
    try {
        if (mode.strictEquals("deletePrototype") || mode.strictEquals("hidePrototype") || mode.strictEquals("revealPrototype") || mode.strictEquals("replacePrototypeValue") || mode.strictEquals("deleteShadow"))
            data(prototype, "nodeParityMutation", b, !mode.strictEquals("revealPrototype"));
        if (mode.strictEquals("deleteOwn") || mode.strictEquals("hideOwn") || mode.strictEquals("revealOwn") || mode.strictEquals("deleteShadow"))
            data(dictionary, "nodeParityMutation", b, !mode.strictEquals("revealOwn") && !mode.strictEquals("deleteShadow"));
        result = attempt(s.get("root"), "_getChildren");
    } catch (...) { restore(); throw; }
    restore(); return finish(s, result);
}
JSValue serialization(const JSValue& r) {
    auto s = state(r); auto n = s.get("root"), a = node("a", 101), b = node("b", 102);
    n.set("single", a); n.set("list", array({a, b})); auto dict = JSValue::object(); dict.set("first", b); n.set("dictionary", dict);
    auto meta = JSValue::object(); meta.set("nodes", JSValue::object()); meta.set("textures", JSValue::object()); meta.set("images", JSValue::object());
    const auto mode = r.get("mode").string(); JSValue result;
    if (mode == "serialize") { auto json = JSValue::object(); json.set("meta", meta); result = attempt(n, "serialize", {json}); auto out = finish(s, result); out.set("json", normal(json)); return out; }
    if (mode == "deserialize") {
        auto json = JSValue::object(); auto inputs = JSValue::object(); inputs.set("single", "uuid-a"); inputs.set("list", array({"uuid-a", "missing", "uuid-b"})); auto object = JSValue::object(); object.set("x", "uuid-b"); inputs.set("dictionary", object); json.set("inputNodes", inputs); json.set("meta", meta); meta.get("nodes").set("uuid-a", a); meta.get("nodes").set("uuid-b", b);
        result = attempt(n, "deserialize", {json}); auto out = finish(s, result); out.set("fields", array({normal(n.get("single")), normal(n.get("list")), normal(n.get("dictionary"))})); return out;
    }
    auto values = array();
    if (mode == "root") call(values, "push", {attempt(n, "toJSON")});
    else if (mode == "string") call(values, "push", {attempt(n, "toJSON", {"root"})});
    else if (mode == "null") call(values, "push", {attempt(n, "toJSON", {nullptr})});
    else { call(values, "push", {attempt(n, "toJSON", {meta})}); call(values, "push", {attempt(n, "toJSON", {meta})}); }
    auto out = finish(s, values); out.set("meta", normal(meta)); return out;
}
JSValue gc() {
    auto keeper = JSValue::object(), result = array(); JSWeakValue weakNode, weakCallback, weakPayload; JSValue live;
    {
        auto n = node(); auto payload = JSValue::object(); payload.set("self", payload);
        auto cb = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { return c[0]; }, {payload});
        call(n, "onFrameUpdate", {cb}); n.set("self", n); payload.set("node", n);
        weakNode = JSWeakValue(n); weakCallback = JSWeakValue(cb); weakPayload = JSWeakValue(payload); live = n;
    }
    keeper.collectGarbage(); call(result, "push", {array({!weakNode.expired(), !weakCallback.expired(), !weakPayload.expired()})});
    live = {}; keeper.collectGarbage(); call(result, "push", {array({!weakNode.expired(), !weakCallback.expired(), !weakPayload.expired()})});
    for (const auto* stage : {"before", "suspended", "exhausted", "returned", "thrown", "nextThrow", "childThrow"}) {
        std::vector<JSWeakValue> weak;
        {
            auto source = JSValue::object(), n = node(), inner = JSValue::object(), iterable = JSValue::object();
            auto next = JSValue::function([](const JSValue& self, const Args&, const Args&) -> JSValue {
                if (self.get("mode").strictEquals("nextThrow")) throw JSThrownValue("err");
                const auto index = self.get("index"); auto inner = self; inner.set("index", index.toNumber() + 1);
                auto record = JSValue::object(); record.set("childNode", "child");
                auto result = JSValue::object(); result.set("value", self.get("mode").strictEquals("childThrow") ? JSValue(nullptr) : record);
                result.set("done", index.toNumber() > 0); return result;
            });
            inner.set("index", 0); inner.set("mode", stage); inner.set("next", next);
            inner.set("return", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue {
                auto result = JSValue::object(); result.set("done", true); return result;
            }));
            iterable.set("inner", inner);
            iterable.set(JSValue::wellKnownSymbol("iterator"), JSValue::function([](const JSValue& self, const Args&, const Args&) -> JSValue { return self.get("inner"); }));
            source.set("value", iterable);
            n.set("_getChildren", JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { return c[0].get("value"); }, {source}));
            live = call(n, "getChildren");
            for (const auto& value : {n, iterable, inner, next}) weak.emplace_back(value);
            try {
                if (std::string(stage) != "before") call(live, "next");
                if (std::string(stage) == "exhausted") call(live, "next");
                if (std::string(stage) == "returned") call(live, "return");
                if (std::string(stage) == "thrown") call(live, "throw", {"outer"});
            } catch (const JSThrownValue&) {} catch (const JSTypeError&) {}
            source.set("value", {});
        }
        const auto snapshot = [&]() {
            auto values = array(); for (const auto& value : weak) call(values, "push", {!value.expired()}); call(result, "push", {values});
        };
        keeper.collectGarbage(); snapshot(); live = {}; keeper.collectGarbage(); snapshot();
    }
    return result;
}
JSValue run(const JSValue& r) {
    const auto kind = r.get("kind").string();
    if (kind == "metadata") return metadata(r); if (kind == "updates") return updates(r);
    if (kind == "children") return children(r); if (kind == "cache") return cache(r); if (kind == "types") return types(r);
    if (kind == "shared") return shared(r); if (kind == "build") return build(r); if (kind == "bindings") return bindings(r);
    if (kind == "traverse") return traverse(r); if (kind == "bindingOrder") return bindingOrder(r);
    if (kind == "evaluationOrder") return evaluationOrder(r); if (kind == "generator") return generator(r);
    if (kind == "generatorProtocol") return generatorProtocol(r);
    if (kind == "enumerationMutation") return enumerationMutation(r);
    if (kind == "serialization") return serialization(r); throw JSTypeError("unknown Node scenario");
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc > 1 && std::string(argv[1]) == "gc") { std::cout << gc().toJSON()->dump(); return 0; }
        const std::string input(std::istreambuf_iterator<char>(std::cin), {}); auto requests = Animation_detail::parseJSON(input), out = array();
        for (std::uint32_t i = 0; i < requests.length(); ++i) call(out, "push", {run(requests.get(std::to_string(i)))});
        std::cout << out.toJSON()->dump();
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
