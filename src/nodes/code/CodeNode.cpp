#include "nodes/code/CodeNode.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLCore.h"
#include "core/JSIterator.h"
#include "core/JSModuleObject.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t index, const JSValue& fallback = {}) { return index < args.size() && !args[index].isUndefined() ? args[index] : fallback; }
struct Invocation { JSValue object, method; JSValue operator()(const Args& args = {}) const { return method.call(object, args); } };
Invocation invoke(const JSValue& object, const char* key) { return {object, object.get(key)}; }
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string(); const auto builder = arg(args, 0);
    if (name == "isCacheable") return false;
    if (name == "setIncludes") { self.set("includes", builder); return self; }
    if (name == "getIncludes") return self.get("includes");
    if (name == "generate") {
        const auto includes = invoke(self, "getIncludes")({builder}); auto iterator = getJSIterator(includes);
        while (const auto include = stepJSIterator(iterator)) {
            try { invoke(*include, "build")({builder, "void"}); }
            catch (...) { closeJSIterator(iterator, std::current_exception()); }
        }
        auto nodeCode = invoke(builder, "getCodeFromNode")({self, invoke(self, "getNodeType")({builder})});
        nodeCode.set("code", self.get("code")); return nodeCode.get("code");
    }
    if (name == "serialize" || name == "deserialize") {
        captures[1].getPrototype().get(name, self).call(self, {builder});
        if (name == "serialize") { auto data = builder; data.set("code", self.get("code")); data.set("language", self.get("language")); }
        else { self.set("code", builder.get("code")); self.set("language", builder.get("language")); }
        return {};
    }
    throw std::logic_error("Unknown CodeNode method");
}
}
JSValue nativeCodeNodeConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isConstructible()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "CodeNode");
    const auto base = nativeNodeConstructor(); auto holder = JSValue::object(nullptr);
    auto constructor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        auto self = captures[0].get("constructor").getPrototype().construct({"code"}, newTarget);
        self.set("isCodeNode", true); self.set("global", true);
        self.set("code", arg(args, 0, "")); self.set("includes", arg(args, 1, JSValue::array())); self.set("language", arg(args, 2, "")); return self;
    }, {holder}, "CodeNode", 0);
    holder.set("constructor", constructor); constructor.setPrototype(base);
    auto prototype = constructor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type; type.getter = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return "CodeNode"; }, {}, "get type", 0);
    type.configurable = true; constructor.defineProperty("type", type);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"isCacheable", 0}, {"setIncludes", 1}, {"getIncludes", 0}, {"generate", 1}, {"serialize", 1}, {"deserialize", 1}}) {
        JSPropertyDescriptor d; d.value = JSValue::function(method, {name, prototype}, name, arity); d.writable = d.configurable = true; prototype.defineProperty(name, d);
    }
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
CodeNode::CodeNode(const JSValue& code, const JSValue& includes, const JSValue& language) : object_(constructor().construct({code, includes, language})) {}
JSValue nativeCodeNodeModule() {
    static thread_local JSWeakValue weak;
    if (const auto module = weak.lock(); module && module->isObject()) return *module;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "CodeNode exports");
    const auto tsl = nativeTSLCore();
    const auto proxy = tsl.get("nodeProxy").call({}, {nativeCodeNodeConstructor()});
    const auto code = proxy.get("setParameterLength").call(proxy, {1, 3});
    auto module = JSValue::object();
    const auto define = [&](const char* name, const JSValue& value) {
        JSPropertyDescriptor descriptor; descriptor.value = value;
        descriptor.writable = descriptor.enumerable = descriptor.configurable = true; module.defineProperty(name, descriptor);
    };
    define("code", code); define("default", nativeCodeNodeConstructor());
    for (const auto* language : {"glsl", "js", "wgsl"}) {
        define(language, JSValue::function([](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            return captures[0].call({}, {args.empty() ? JSValue{} : args[0], args.size() < 2 ? JSValue{} : args[1], captures[1]});
        }, {code, language}, language, 2));
    }
    module.retainForHeapLifetime(); weak = JSWeakValue(module); return module;
}
JSValue CodeNode::setIncludes(const JSValue& includes) const { return invoke(object_, "setIncludes")({includes}); }
JSValue CodeNode::getIncludes(const JSValue& builder) const { return invoke(object_, "getIncludes")({builder}); }
JSValue CodeNode::generate(const JSValue& builder) const { return invoke(object_, "generate")({builder}); }
