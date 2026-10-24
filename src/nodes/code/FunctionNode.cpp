#include "nodes/code/FunctionNode.h"
#include "nodes/code/CodeNode.h"
#include "core/JSModuleObject.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t index, const JSValue& fallback = {}) { return index < args.size() && !args[index].isUndefined() ? args[index] : fallback; }
struct Invocation { JSValue object, method; JSValue operator()(const Args& args = {}) const { return method.call(object, args); } };
Invocation invoke(const JSValue& object, const char* name) { return {object, object.get(name)}; }
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string(); const auto builder = arg(args, 0);
    if (name == "generateNodeType") return invoke(self, "getNodeFunction")({builder}).get("type");
    if (name == "getInputs") return invoke(self, "getNodeFunction")({builder}).get("inputs");
    if (name == "getMemberType") {
        const auto type = invoke(self, "getNodeType")({builder}); const auto structure = invoke(builder, "getStructTypeNode")({type});
        return invoke(structure, "getMemberType")({builder, arg(args, 1)});
    }
    if (name == "getNodeFunction") {
        auto data = invoke(builder, "getDataFromNode")({self}); auto function = data.get("nodeFunction");
        if (function.isUndefined()) { function = invoke(builder.get("parser"), "parseFunction")({self.get("code")}); data.set("nodeFunction", function); }
        return function;
    }
    if (name == "generate") {
        captures[1].getPrototype().get("generate", self).call(self, {builder});
        const auto function = invoke(self, "getNodeFunction")({builder}), functionName = function.get("name"), type = function.get("type");
        auto code = invoke(builder, "getCodeFromNode")({self, type});
        if (!functionName.strictEquals("")) {
            auto data = invoke(builder, "getDataFromNode")({self});
            if (!data.get("declarationRegistered").strictEquals(true)) {
                code.set("name", functionName); invoke(builder, "registerDeclaration")({code}); data.set("declarationRegistered", true);
            }
        }
        const auto property = invoke(builder, "getPropertyName")({code});
        const auto source = invoke(invoke(self, "getNodeFunction")({builder}), "getCode")({property});
        code.set("code", source.toPrimitive().toString() + "\n");
        const auto output = arg(args, 1);
        if (output.strictEquals("property")) return property;
        return invoke(builder, "format")({property.toString() + "()", type, output});
    }
    throw std::logic_error("Unknown FunctionNode method");
}
}
JSValue nativeFunctionNodeConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isConstructible()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "FunctionNode");
    const auto base = nativeCodeNodeConstructor(); auto holder = JSValue::object(nullptr);
    auto constructor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        return captures[0].get("constructor").getPrototype().construct({arg(args, 0, ""), arg(args, 1, JSValue::array()), arg(args, 2, "")}, newTarget);
    }, {holder}, "FunctionNode", 0);
    holder.set("constructor", constructor); constructor.setPrototype(base); auto prototype = constructor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type; type.getter = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return "FunctionNode"; }, {}, "get type", 0); type.configurable = true; constructor.defineProperty("type", type);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"generateNodeType", 1}, {"getMemberType", 2}, {"getInputs", 1}, {"getNodeFunction", 1}, {"generate", 2}}) {
        JSPropertyDescriptor d; d.value = JSValue::function(method, {name, prototype}, name, arity); d.writable = d.configurable = true; prototype.defineProperty(name, d);
    }
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
FunctionNode::FunctionNode(const JSValue& code, const JSValue& includes, const JSValue& language) : object_(constructor().construct({code, includes, language})) {}
JSValue FunctionNode::getNodeFunction(const JSValue& builder) const { return invoke(object_, "getNodeFunction")({builder}); }
JSValue FunctionNode::getInputs(const JSValue& builder) const { return invoke(object_, "getInputs")({builder}); }
JSValue FunctionNode::generate(const JSValue& builder, const JSValue& output) const { return invoke(object_, "generate")({builder, output}); }
