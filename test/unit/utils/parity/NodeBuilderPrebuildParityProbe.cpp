#include "animation/AnimationUtils.h"
#include "nodes/core/NodeBuilder.h"
#include "utils.h"
#include <algorithm>
#include <iostream>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue call(const JSValue& self, const char* name, const Args& args = {}) { return self.get(name).call(self, args); }
JSValue array(const Args& values = {}) { auto out = JSValue::array(); for (const auto& value : values) call(out, "push", {value}); return out; }
void event(const JSValue& state, const Args& values) { call(state.get("events"), "push", {array(values)}); }
void data(JSValue object, const JSValue& key, const JSValue& value, bool enumerable = true) { JSPropertyDescriptor d; d.value = value; d.writable = d.configurable = true; d.enumerable = enumerable; object.defineProperty(key, d); }
JSValue enc(const JSValue& value) { if (value.isUndefined()) return "#undefined"; if (value.isSymbol()) return "#symbol"; if (value.isCallable()) return "#function"; if (value.isObject()) return "#object"; return value; }
JSValue decode(const JSValue& value) { return value.isObject() && value.get("tag").strictEquals("undefined") ? JSValue{} : value; }
JSValue view(const JSValue& value) {
    if (!value.isObject()) return enc(value); auto result = array();
    for (const auto& key : value.ownPropertyKeys()) { const auto d = value.getOwnPropertyDescriptor(key).value(); call(result, "push", {array({key.isSymbol() ? JSValue("#symbol") : key, d.value ? enc(*d.value) : JSValue("#accessor"), d.writable.value_or(false), d.enumerable.value_or(false), d.configurable.value_or(false)})}); }
    return result;
}
void fail(const JSValue& state, const std::string& label, double count) { const auto at = state.get("r").get("throwAt"); if (at.strictEquals(label) || at.strictEquals(label + ":" + JSValue(count).toString())) throw JSTypeError(label); }
double count(const JSValue& state, const std::string& label) { auto counts = state.get("counts"); const auto old = counts.get(label); const auto n = old.isUndefined() ? 1 : old.toNumber() + 1; counts.set(label, n); return n; }
JSValue select(const JSValue& state, const JSValue& value) {
    if (value.strictEquals("null")) return nullptr; if (value.strictEquals("undefined")) return {}; if (value.strictEquals("false")) return false; if (value.strictEquals("zero")) return 0;
    return state.get("nodes").get(value);
}
void observe(const JSValue& state, JSValue object, const char* key, const std::string& label, const JSValue& value, const JSValue& mode = {}) {
    JSPropertyDescriptor d; d.configurable = d.enumerable = true;
    d.getter = JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue {
        const auto n = count(c[0], c[1].toString()); event(c[0], {"get", c[1], n, self.sameValue(c[2])}); fail(c[0], c[1].toString(), n);
        if (!c[4].isUndefined()) {
            auto sequence = c[0].get("r").get(c[4]); if (!sequence.toBoolean()) sequence = array({c[4].strictEquals("rendererSequence") ? JSValue("renderer") : JSValue("material")});
            return select(c[0], sequence.get(JSValue(std::min(n - 1, sequence.get("length").toNumber() - 1)).toString()));
        }
        return c[3];
    }, {state, label, object, value, mode}); object.defineProperty(key, d);
}
JSValue context(const JSValue& state, const std::string& label) {
    auto object = JSValue::object(); data(object, "2", label + "-index");
    JSPropertyDescriptor first; first.configurable = first.enumerable = true;
    first.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
        const auto label = c[1].toString(); event(c[0], {"spread", c[1], "first"}); fail(c[0], label + ".first", 1);
        const auto r = c[0].get("r"); if (r.get("mutateSource").strictEquals(c[1])) {
            auto object = c[2]; const auto mutation = r.get("mutation");
            if (mutation.strictEquals("delete")) object.erase("later");
            if (mutation.strictEquals("replace")) data(object, "later", "replaced");
            if (mutation.strictEquals("hidden")) data(object, "hidden", "visible-now");
            if (mutation.strictEquals("symbol")) object.erase(c[0].get("symbol"));
            object.set("added", "added-late");
        }
        return label + "-first";
    }, {state, label, object}); object.defineProperty("first", first); data(object, "overlap", label);
    JSPropertyDescriptor later; later.configurable = later.enumerable = true;
    later.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { event(c[0], {"spread", c[1], "later"}); fail(c[0], c[1].toString() + ".later", 1); return c[1].toString() + "-later"; }, {state, label}); object.defineProperty("later", later);
    data(object, "hidden", "hidden", false); data(object, "__proto__", label + "-proto");
    JSPropertyDescriptor symbol; symbol.configurable = true; symbol.enumerable = !state.get("r").get("symbolEnumerable").strictEquals(false);
    symbol.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { event(c[0], {"spread", c[1], "#symbol"}); fail(c[0], c[1].toString() + ".symbol", 1); return c[1].toString() + "-symbol"; }, {state, label}); object.defineProperty(state.get("symbol"), symbol); return object;
}
JSValue contextData(const JSValue& state, const std::string& label) {
    const auto mode = state.get("r").get(label + "Data"); if (mode.strictEquals("null")) return nullptr; if (mode.strictEquals("undefined")) return {}; if (mode.strictEquals("string")) return "A\xF0\x9F\x98\x80"; if (mode.strictEquals("number")) return 4;
    return state.get(label + "Data");
}
void method(const JSValue& state, JSValue object, const char* key, const std::string& label) {
    const auto fn = JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
        const auto label = c[1].toString(); const auto n = count(c[0], label + ":call"); auto arguments = array(); for (const auto& arg : args) call(arguments, "push", {enc(arg)});
        event(c[0], {"call", c[1], self.sameValue(c[2]), arguments}); fail(c[0], label + ":call", n);
        if (label == "renderer.getFlowContextData" || label == "other.getFlowContextData") return contextData(c[0], "renderer");
        if (label == "material.getFlowContextData") return contextData(c[0], "material");
        if (label == "library.fromMaterial") { const auto value = c[0].get("r").get("fromResult"); if (value.strictEquals("false")) return false; if (value.strictEquals("undefined")) return {}; return c[0].get("nodeMaterial"); }
        if (label == "nodeMaterial.build") { auto current = c[0].get("current"); event(c[0], {"buildState", arg(args, 0).sameValue(c[0].get("self")), view(current)}); current.set("built", true); return {}; }
        call(c[0].get("flows"), "push", {array({arg(args, 0), arg(args, 1).sameValue(c[0].get("compute"))})}); return {};
    }, {state, label, object}); observe(state, object, key, label, fn);
}
JSValue run(const JSValue& r) {
    auto state = JSValue::object(nullptr); state.set("r", r); state.set("events", array()); state.set("counts", JSValue::object()); state.set("symbol", JSValue::symbol("context")); state.set("nodes", JSValue::object()); state.set("flows", array());
    for (const auto* key : {"self", "renderer", "material", "library", "nodeMaterial", "compute"}) state.set(key, JSValue::object());
    const auto self = state.get("self"), renderer = state.get("renderer"), material = state.get("material"), library = state.get("library"), nodeMaterial = state.get("nodeMaterial");
    const auto initial = context(state, "initial"); state.set("current", initial); state.set("rendererData", context(state, "renderer")); state.set("materialData", context(state, "material"));
    for (const auto* label : {"renderer", "material", "other"}) {
        auto object = JSValue::object(), nodes = state.get("nodes"); nodes.set(label, object); const auto flag = std::string(label) + "Flag";
        observe(state, object, "isContextNode", std::string(label) + ".isContextNode", r.hasOwn(flag) ? decode(r.get(flag)) : JSValue(true));
        method(state, object, "getFlowContextData", std::string(label) + ".getFlowContextData");
    }
    observe(state, renderer, "contextNode", "renderer.contextNode", {}, "rendererSequence"); observe(state, material, "contextNode", "material.contextNode", {}, "materialSequence");
    observe(state, renderer, "library", "renderer.library", library); method(state, library, "fromMaterial", "library.fromMaterial"); method(state, nodeMaterial, "build", "nodeMaterial.build");
    observe(state, self, "renderer", "self.renderer", renderer);
    const auto m = r.get("material"); const auto materialValue = m.strictEquals("null") ? JSValue(nullptr) : m.strictEquals("undefined") ? JSValue{} : m.strictEquals("false") ? JSValue(false) : m.strictEquals("zero") ? JSValue(0) : material;
    observe(state, self, "material", "self.material", materialValue);
    JSPropertyDescriptor d; d.configurable = d.enumerable = true;
    d.getter = JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue { const auto n = count(c[0], "self.context"); event(c[0], {"get", "self.context", n, self.sameValue(c[0].get("self"))}); fail(c[0], "self.context", n); return c[0].get("current"); }, {state});
    d.setter = JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue { const auto n = count(c[0], "self.context:set"); event(c[0], {"set", "self.context", n, self.sameValue(c[0].get("self")), view(arg(args, 0))}); fail(c[0], "self.context:set", n); auto state = c[0]; state.set("current", arg(args, 0)); return {}; }, {state});
    auto mutableSelf = self; mutableSelf.defineProperty("context", d); observe(state, self, "compute", "self.compute", state.get("compute")); method(state, self, "addFlow", "self.addFlow");
    const auto saved = ThreeUtils::getConsoleFunction(); ThreeUtils::setConsoleFunction(JSValue::function([](const JSValue&, const Args& args, const Args& c) -> JSValue { auto values = array(); for (const auto& arg : args) call(values, "push", {enc(arg)}); event(c[0], {"console", values}); fail(c[0], "console", 1); return {}; }, {state}));
    JSValue result;
    try { try { result = enc(nativeNodeBuilderConstructor().get("prototype").get("prebuild").call(self)); } catch (const JSTypeError&) { result = JSValue::object(); result.set("error", "TypeError"); } catch (const std::exception&) { result = JSValue::object(); result.set("error", "Error"); } }
    catch (...) { ThreeUtils::setConsoleFunction(saved); throw; }
    ThreeUtils::setConsoleFunction(saved); const auto current = state.get("current"); auto out = JSValue::object(); out.set("result", result); out.set("events", state.get("events")); out.set("context", view(current)); out.set("initialIdentity", current.sameValue(initial)); out.set("flows", state.get("flows")); out.set("prototypeOrdinary", current.getPrototype().sameValue(JSValue::intrinsicObjectConstructor().get("prototype"))); return out;
}
}
int main() {
    try { const std::string input(std::istreambuf_iterator<char>(std::cin), {}); const auto requests = Animation_detail::parseJSON(input); auto out = array();
        for (std::uint32_t i = 0; i < requests.length(); ++i) call(out, "push", {run(requests.get(std::to_string(i)))}); std::cout << out.toJSON()->dump();
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
