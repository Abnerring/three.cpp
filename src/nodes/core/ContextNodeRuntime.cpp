#include "nodes/core/ContextNode.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLCore.h"
#include "core/JSIterator.h"
#include "core/JSModuleObject.h"
#include "utils.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& a, unsigned i, const JSValue& fallback = {}) { return i < a.size() && !a[i].isUndefined() ? a[i] : fallback; }
struct Invocation { JSValue object, method; JSValue operator()(const Args& a = {}) const { return method.call(object, a); } };
Invocation invoke(const JSValue& o, const char* key) { return {o, o.get(key)}; }
JSValue property(const char* key, const JSValue& value) { auto o = JSValue::object(); o.set(key, value); return o; }
JSValue method(const JSValue& self, const Args& a, const Args& captures) {
    const auto name = captures[0].string(); const auto builder = arg(a, 0);
    if (name == "isCacheable") return false;
    if (name == "getScope") return invoke(self.get("node"), "getScope")();
    if (name == "generateNodeType") return invoke(self.get("node"), "getNodeType")({builder});
    if (name == "getMemberType") return invoke(self.get("node"), "getMemberType")({builder, arg(a, 1)});
    if (name == "getFlowContextData") {
        const auto children = JSValue::array();
        invoke(self, "traverse")({JSValue::function([](const JSValue&, const Args& a, const Args& c) -> JSValue {
            const auto node = arg(a, 0); if (node.get("isContextNode").strictEquals(true)) { const auto push = invoke(c[0], "push"); push({node.get("value")}); } return {};
        }, {children}, "", 1)});
        const auto assign = invoke(JSValue::intrinsicObjectConstructor(), "assign"); Args values{JSValue::object()};
        auto iterator = getJSIterator(children); while (const auto value = stepJSIterator(iterator)) values.push_back(*value); return assign(values);
    }
    if (name == "analyze") {
        const auto count = invoke(builder, "increaseUsage")({self});
        if (count.strictEquals(1)) { const auto add = invoke(builder, "addContext"); const auto previous = add({self.get("value")}); invoke(self.get("node"), "build")({builder, self}); invoke(builder, "setContext")({previous}); } return {};
    }
    if (name == "setup" || name == "generate") {
        const auto add = invoke(builder, "addContext"); const auto previous = add({self.get("value")});
        const auto snippet = name == "generate" ? invoke(self.get("node"), "build")({builder, arg(a, 1)}) : invoke(self.get("node"), "build")({builder});
        invoke(builder, "setContext")({previous}); return name == "generate" ? snippet : JSValue{};
    }
    throw std::logic_error("Unknown ContextNode method");
}
}
JSValue nativeContextNodeConstructor() {
    static thread_local JSWeakValue weak; if (const auto value = weak.lock(); value && value->isConstructible()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "ContextNode"); const auto base = nativeNodeConstructor(); auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor([](const JSValue& target, const Args& a, const Args& c) -> JSValue {
        auto self = c[0].get("constructor").getPrototype().construct({}, target); self.set("isContextNode", true); self.set("node", arg(a, 0, nullptr)); self.set("value", arg(a, 1, JSValue::object())); return self;
    }, {holder}, "ContextNode", 0);
    holder.set("constructor", ctor); ctor.setPrototype(base); auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type; type.getter = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return "ContextNode"; }, {}, "get type", 0); type.configurable = true; ctor.defineProperty("type", type);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"isCacheable",0},{"getScope",0},{"generateNodeType",1},{"getFlowContextData",0},{"getMemberType",2},{"analyze",1},{"setup",1},{"generate",2}}) {
        JSPropertyDescriptor d; d.value = JSValue::function(method, {name}, name, arity); d.writable = d.configurable = true; prototype.defineProperty(name, d);
    }
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
namespace {JSWeakValue& contextBindings(){static thread_local JSWeakValue weak;return weak;}}
JSValue nativeContextNodeModule() {
    static thread_local JSWeakValue weak; if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "ContextNode exports"); const auto ctor = nativeContextNodeConstructor();
    const auto context = JSValue::function([](const JSValue&, const Args& a, const Args& c) -> JSValue {
        auto node = arg(a, 0, nullptr); auto value = arg(a, 1, JSValue::object());
        if (node.isNull() || !node.get("isNode").strictEquals(true)) { value = node.toBoolean() ? node : value; node = nullptr; }
        return c[0].construct({node, value});
    }, {ctor}, "context", 0);
    const auto uniform = JSValue::function([](const JSValue&, const Args& a, const Args& c) -> JSValue { return c[0].call({}, {arg(a, 0), property("uniformFlow", true)}); }, {context}, "uniformFlow", 1);
    const auto setName = JSValue::function([](const JSValue&, const Args& a, const Args& c) -> JSValue { return c[0].call({}, {arg(a, 0), property("nodeName", arg(a, 1))}); }, {context}, "setName", 2);
    const auto shadow = JSValue::constructor([](const JSValue&, const Args& a, const Args& c) -> JSValue {
        const auto callback = JSValue::function([](const JSValue&, const Args& a, const Args& c) -> JSValue {
            const auto input = arg(a, 0); const auto light = input.get("light"); const auto color = input.get("shadowColorNode"); return c[1].strictEquals(light) ? invoke(color, "mul")({c[0]}) : color;
        }, {arg(a, 0), arg(a, 1)}, "getShadow", 1);
        return c[0].call({}, {arg(a, 2, nullptr), property("getShadow", callback)});
    }, {context}, "builtinShadowContext", 2, JSValue::ConstructorKind::Callable);
    const auto ao = JSValue::constructor([](const JSValue&, const Args& a, const Args& c) -> JSValue {
        const auto callback = JSValue::function([](const JSValue&, const Args& a, const Args& c) -> JSValue {
            const auto input = arg(a, 0); const auto material = arg(a, 1).get("material"); if (material.get("transparent").strictEquals(true)) return input; return !input.isNull() ? invoke(input, "mul")({c[0]}) : c[0];
        }, {arg(a, 0)}, "getAO", 2);
        return c[0].call({}, {arg(a, 1, nullptr), property("getAO", callback)});
    }, {context}, "builtinAOContext", 1, JSValue::ConstructorKind::Callable);
    const auto gi = JSValue::constructor([](const JSValue&, const Args& a, const Args& c) -> JSValue {
        const auto ao = invoke(arg(a, 0), "max")({1e-3}); const auto illumination = arg(a, 1); auto value = JSValue::object();
        value.set("getAO", JSValue::function([](const JSValue&, const Args& a, const Args& c) -> JSValue {
            const auto input = arg(a, 0); const auto material = arg(a, 1).get("material"); if (material.get("transparent").strictEquals(true)) return input; return !input.isNull() ? invoke(input, "mul")({c[0]}) : c[0];
        }, {ao}, "getAO", 2));
        value.set("getGI", JSValue::function([](const JSValue&, const Args& a, const Args& c) -> JSValue {
            const auto input = arg(a, 0); const auto material = arg(a, 1).get("material"); if (material.get("transparent").strictEquals(true)) return input;
            const auto gi = invoke(c[1], "div")({c[0]}); return !input.isNull() ? invoke(input, "add")({gi}) : gi;
        }, {ao, illumination}, "getGI", 2));
        return c[0].call({}, {arg(a, 2, nullptr), value});
    }, {context}, "builtinGIContext", 2, JSValue::ConstructorKind::Callable);
    const auto label = JSValue::constructor([](const JSValue&, const Args& a, const Args& c) -> JSValue { ThreeUtils::warn({"TSL: \"label()\" has been deprecated. Use \"setName()\" instead."}); return c[0].call({}, {arg(a, 0), arg(a, 1)}); }, {setName}, "label", 2, JSValue::ConstructorKind::Callable);
    auto module = JSValue::object();
    for (const auto& [name, value] : std::initializer_list<std::pair<const char*, JSValue>>{{"builtinAOContext",ao},{"builtinGIContext",gi},{"builtinShadowContext",shadow},{"context",context},{"default",ctor},{"label",label},{"setName",setName},{"uniformFlow",uniform}}) module.set(name, value);
    const auto add = nativeTSLCoreBinding("addMethodChaining");
    add.call({}, {"context",context}); add.call({}, {"label",label}); add.call({}, {"uniformFlow",uniform}); add.call({}, {"setName",setName});
    add.call({}, {"builtinShadowContext", JSValue::function([](const JSValue&, const Args& a, const Args& c) -> JSValue { return c[0].call({}, {arg(a, 1), arg(a, 2), arg(a, 0)}); }, {shadow}, "", 3)});
    add.call({}, {"builtinAOContext", JSValue::function([](const JSValue&, const Args& a, const Args& c) -> JSValue { return c[0].call({}, {arg(a, 1), arg(a, 0)}); }, {ao}, "", 2)});
    add.call({}, {"builtinGIContext", JSValue::function([](const JSValue&, const Args& a, const Args& c) -> JSValue { return c[0].call({}, {arg(a, 1), arg(a, 2), arg(a, 0)}); }, {gi}, "", 3)});
    auto bindings=JSValue::object(nullptr);for(const auto& name:module.ownPropertyNames())bindings.set(name,module.get(name));
    bindings.retainForHeapLifetime();contextBindings()=JSWeakValue(bindings);
    module.retainForHeapLifetime(); weak = JSWeakValue(module); return module;
}
JSValue nativeContextNodeBinding(const std::string& name){const auto keeper=nativeContextNodeModule();return contextBindings().lock()->get(name);}
