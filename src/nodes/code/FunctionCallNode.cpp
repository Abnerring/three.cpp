// Ported from pinned three.js src/nodes/code/FunctionCallNode.js.
#include "nodes/code/FunctionCallNode.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLCore.h"
#include "core/JSIterator.h"
#include "core/JSModuleObject.h"
#include "utils.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t index) { return index < args.size() ? args[index] : JSValue{}; }
struct Invocation { JSValue object, method; JSValue operator()(const Args& args = {}) const { return method.call(object, args); } };
Invocation invoke(const JSValue& object, const char* key) { return {object, object.get(key)}; }
JSValue rest(const Args& args) {
    auto result = JSValue::array(static_cast<std::uint32_t>(args.size()));
    for (std::size_t i = 0; i < args.size(); ++i) {
        JSPropertyDescriptor d; d.value = args[i]; d.writable = d.enumerable = d.configurable = true;
        result.defineProperty(std::to_string(i), d);
    }
    return result;
}
JSValue generateInput(const JSValue& node, const JSValue& input, const JSValue& builder) {
    const auto type = input.get("type");
    if (type.strictEquals("pointer")) {
        const auto output = invoke(node, "build")({builder});
        return "&" + output.toPrimitive().toString();
    }
    return invoke(node, "build")({builder, type});
}
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string(); const auto builder = arg(args, 0);
    if (name == "setParameters") { self.set("parameters", builder); return self; }
    if (name == "getParameters") return self.get("parameters");
    if (name == "generateNodeType") return invoke(self.get("functionNode"), "getNodeType")({builder});
    if (name == "getMemberType") return invoke(self.get("functionNode"), "getMemberType")({builder, arg(args, 1)});
    if (name == "generate") {
        auto params = JSValue::array(); const auto functionNode = self.get("functionNode");
        const auto inputs = invoke(functionNode, "getInputs")({builder}); auto parameters = self.get("parameters");
        const auto array = JSValue::intrinsicArrayConstructor();
        if (invoke(array, "isArray")({parameters}).toBoolean()) {
            const auto length = parameters.get("length"); const auto inputLength = inputs.get("length");
            if (length.toNumber() > inputLength.toNumber()) {
                ThreeUtils::error({"TSL: The number of provided parameters exceeds the expected number of inputs in 'Fn()'."});
                parameters.set("length", inputs.get("length"));
            } else {
                const auto nextLength = parameters.get("length"); const auto nextInputLength = inputs.get("length");
                if (nextLength.toNumber() < nextInputLength.toNumber()) {
                    ThreeUtils::error({"TSL: The number of provided parameters is less than the expected number of inputs in 'Fn()'."});
                    while (true) {
                        const auto currentLength = parameters.get("length"); const auto currentInputLength = inputs.get("length");
                        if (!(currentLength.toNumber() < currentInputLength.toNumber())) break;
                        const auto push = invoke(parameters, "push"); const auto zero = captures[1].call({}, {0}); push({zero});
                    }
                }
            }
            for (double i = 0; i < parameters.get("length").toNumber(); ++i) {
                const auto push = invoke(params, "push"); const auto node = parameters.get(JSValue(i)); const auto input = inputs.get(JSValue(i));
                const auto output = generateInput(node, input, builder); push({output});
            }
        } else {
            auto iterator = getJSIterator(inputs);
            while (const auto input = stepJSIterator(iterator)) {
                try {
                    const auto node = parameters.get(input->get("name"));
                    if (!node.isUndefined()) {
                        const auto push = invoke(params, "push"); const auto output = generateInput(node, *input, builder); push({output});
                    } else {
                        const auto missingName = input->get("name").toString();
                        ThreeUtils::error({"TSL: Input '" + missingName + "' not found in 'Fn()'."});
                        const auto push = invoke(params, "push"); const auto zero = captures[1].call({}, {0});
                        const auto output = generateInput(zero, *input, builder); push({output});
                    }
                } catch (...) { closeJSIterator(iterator, std::current_exception()); }
            }
        }
        const auto functionName = invoke(functionNode, "build")({builder, "property"});
        const auto functionText = functionName.toString(); const auto joined = invoke(params, "join")({", "});
        return functionText + "( " + joined.toString() + " )";
    }
    throw std::logic_error("Unknown FunctionCallNode method");
}
}
JSValue nativeFunctionCallNodeConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto ctor = weak.lock(); ctor && ctor->isConstructible()) return *ctor;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "FunctionCallNode");
    const auto base = nativeNodeConstructor(); auto holder = JSValue::object(nullptr);
    // float is an ESM import binding. Capture the resolved intrinsic once rather
    // than observing subsequent writes to the host module namespace facade.
    const auto floatFactory = nativeTSLCore().get("float");
    auto ctor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        auto self = captures[0].get("constructor").getPrototype().construct({}, newTarget);
        const auto functionNode = arg(args, 0), parameters = arg(args, 1);
        self.set("functionNode", functionNode.isUndefined() ? JSValue(nullptr) : functionNode);
        self.set("parameters", parameters.isUndefined() ? JSValue::object() : parameters); return self;
    }, {holder}, "FunctionCallNode", 0);
    holder.set("constructor", ctor); ctor.setPrototype(base); auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type; type.getter = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return "FunctionCallNode"; }, {}, "get type", 0);
    type.configurable = true; ctor.defineProperty("type", type);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"setParameters", 1}, {"getParameters", 0}, {"generateNodeType", 1}, {"getMemberType", 2}, {"generate", 1}}) {
        JSPropertyDescriptor d; d.value = JSValue::function(method, {name, floatFactory}, name, arity); d.writable = d.configurable = true; prototype.defineProperty(name, d);
    }
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
namespace {JSWeakValue& functionCallBindings(){static thread_local JSWeakValue weak;return weak;}}
JSValue nativeFunctionCallNodeModule() {
    static thread_local JSWeakValue weak;
    if (const auto module = weak.lock(); module && module->isObject()) return *module;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "FunctionCallNode exports");
    const auto tsl = nativeTSLCore(); const auto ctor = nativeFunctionCallNodeConstructor();
    auto module = JSValue::object();
    const auto call = JSValue::function([](const JSValue&, const Args& args, const Args& captures) -> JSValue {
        const auto func = arg(args, 0); auto params = rest(args.empty() ? Args{} : Args(args.begin() + 1, args.end()));
        bool positional = params.get("length").toNumber() > 1;
        if (!positional && params.get("0").toBoolean()) positional = params.get("0").get("isNode").strictEquals(true);
        const auto processed = captures[positional ? 1 : 2].call({}, {positional ? params : params.get("0")});
        const auto functionNode = captures[3].call({}, {func}); return captures[0].construct({functionNode, processed});
    }, {ctor, nativeTSLCoreBinding("nodeArray"), nativeTSLCoreBinding("nodeObjects"), nativeTSLCoreBinding("nodeObject")}, "call", 1);
    for (const auto& [name, value] : std::initializer_list<std::pair<const char*, JSValue>>{{"call", call}, {"default", ctor}}) {
        JSPropertyDescriptor d; d.value = value; d.writable = d.enumerable = d.configurable = true; module.defineProperty(name, d);
    }
    nativeTSLCoreBinding("addMethodChaining").call({}, {"call", call});
    auto bindings=JSValue::object(nullptr);for(const auto& name:module.ownPropertyNames())bindings.set(name,module.get(name));
    bindings.retainForHeapLifetime();functionCallBindings()=JSWeakValue(bindings);
    module.retainForHeapLifetime(); weak = JSWeakValue(module); return module;
}
JSValue nativeFunctionCallNodeBinding(const std::string& name){const auto keeper=nativeFunctionCallNodeModule();return functionCallBindings().lock()->get(name);}
FunctionCallNode::FunctionCallNode(const JSValue& functionNode, const JSValue& parameters) : object_(constructor().construct({functionNode, parameters})) {}
JSValue FunctionCallNode::setParameters(const JSValue& value) const { return invoke(object_, "setParameters")({value}); }
JSValue FunctionCallNode::getParameters() const { return invoke(object_, "getParameters")(); }
JSValue FunctionCallNode::generate(const JSValue& builder) const { return invoke(object_, "generate")({builder}); }
