#include "nodes/core/StackNode.h"
#include "nodes/core/Node.h"
#include "nodes/core/StackTrace.h"
#include "core/JSMap.h"
#include "core/JSIterator.h"
#include "core/JSModuleObject.h"
#include "utils.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
struct Invocation {
    JSValue object, method;
    JSValue operator()(const Args& args = {}) const { return method.call(object, args); }
};
Invocation invoke(const JSValue& object, const char* name) { return {object, object.get(name)}; }
JSValue state() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    auto object = JSValue::object(nullptr); object.set("currentStack", nullptr); object.retainForHeapLifetime(); weak = JSWeakValue(object); return object;
}
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto& name = captures[0].string(); const auto builder = arg(args, 0);
    if (name == "isCacheable") return false;
    if (name == "getElementType" || name == "generateNodeType" || name == "getMemberType") {
        if (!self.get("outputNode").toBoolean()) return "void";
        const auto output = self.get("outputNode");
        if (name == "getMemberType") return invoke(output, "getMemberType")({builder, arg(args, 1)});
        return invoke(output, name == "generateNodeType" ? "getNodeType" : "getElementType")({builder});
    }
    if (name == "addToStack") {
        const auto node = arg(args, 0); auto index = arg(args, 1); if (index.isUndefined()) index = -1;
        if (!node.get("isNode").strictEquals(true)) { ThreeUtils::error({"TSL: Invalid node added to stack.", nativeStackTraceConstructor().construct()}); return self; }
        if (index.strictEquals(-1)) {
            if (self.get("_currentNode").toBoolean()) {
                auto data = invoke(self.get("_nodeDataLibrary"), "get")({self.get("_currentNode")});
                if (data.isUndefined()) {
                    data = JSValue::object(); JSPropertyDescriptor delta; delta.value = 0; delta.writable = delta.enumerable = delta.configurable = true; data.defineProperty("delta", delta);
                    invoke(self.get("_nodeDataLibrary"), "set")({self.get("_currentNode"), data});
                }
                data.set("delta", data.get("delta").toNumber() + 1);
                const auto position = invoke(self.get("nodes"), "indexOf")({self.get("_currentNode")});
                const auto deltaValue = data.get("delta");
                const auto pos = position.toPrimitive();
                const auto delta = deltaValue.toPrimitive();
                index = pos.type() == JSValue::Type::String || delta.type() == JSValue::Type::String ? JSValue(pos.toString() + delta.toString()) : JSValue(pos.toNumber() + delta.toNumber());
            } else index = self.get("nodes").get("length");
        }
        invoke(self.get("nodes"), "splice")({index, 0, node}); return self;
    }
    if (name == "addToStackBefore") {
        const auto index = self.get("_currentNode").toBoolean() ? invoke(self.get("nodes"), "indexOf")({self.get("_currentNode")}) : JSValue(0);
        return invoke(self, "addToStack")({arg(args, 0), index});
    }
    if (name == "If" || name == "ElseIf" || name == "Else" || name == "Switch" || name == "Case")
        throw std::logic_error("StackNode conditional methods require the pending ShaderNode/ConditionalNode TSL implementation");
    if (name == "Default") { invoke(self, "Else")({arg(args, 0)}); return self; }
    if (name == "setup") {
        auto properties = invoke(builder, "getNodeProperties")({self}); double index = 0;
        auto iterator = getJSIterator(invoke(self, "getChildren")());
        while (const auto child = stepJSIterator(iterator)) {
            try {
                if (child->get("isVarNode").toBoolean() && invoke(*child, "isIntent")({builder}).toBoolean())
                    if (!invoke(*child, "isAssign")({builder}).strictEquals(true)) continue;
                const auto key = "node" + JSValue(index++).toString(); properties.set(key, *child);
            } catch (...) { closeJSIterator(iterator, std::current_exception()); }
        }
        const auto output = properties.get("outputNode"); return output.toBoolean() ? output : JSValue(nullptr);
    }
    if (name == "build") {
        const auto previousStack = nativeGetCurrentStack(); const auto stage = builder.get("buildStage");
        nativeSetCurrentStack(self); invoke(builder, "setActiveStack")({self});
        for (double i = 0; i < self.get("nodes").get("length").toNumber(); ++i) {
            const auto node = self.get("nodes").get(JSValue(i)); const auto previousNode = self.get("_currentNode"); self.set("_currentNode", node);
            if (node.get("isVarNode").toBoolean() && invoke(node, "isIntent")({builder}).toBoolean())
                if (!invoke(node, "isAssign")({builder}).strictEquals(true)) continue;
            if (stage.strictEquals("setup")) invoke(node, "build")({builder});
            else if (stage.strictEquals("analyze")) invoke(node, "build")({builder, self});
            else if (stage.strictEquals("generate")) {
                const auto stages = invoke(builder, "getDataFromNode")({node, "any"}).get("stages");
                const auto parents = stages.toBoolean() ? stages.get(builder.get("shaderStage")) : stages;
                if (node.get("isVarNode").toBoolean() && parents.toBoolean() && parents.get("length").strictEquals(1) && parents.get("0").toBoolean() && parents.get("0").get("isStackNode").toBoolean()) continue;
                invoke(node, "build")({builder, "void"});
            }
            self.set("_currentNode", previousNode);
        }
        JSValue result;
        Args parameters{builder}; if (args.size() > 1) parameters.insert(parameters.end(), args.begin() + 1, args.end());
        if (self.get("outputNode").toBoolean()) {
            const auto value = invoke(self.get("outputNode"), "build")(parameters);
            if (!builder.get("buildStage").strictEquals("generate") || !invoke(self.get("outputNode"), "getNodeType")({builder}).strictEquals("void")) result = value;
        } else result = nativeStackNodeConstructor().get("prototype").getPrototype().get("build", self).call(self, parameters);
        nativeSetCurrentStack(previousStack); invoke(builder, "removeActiveStack")({self}); return result;
    }
    throw std::logic_error("Unknown StackNode method");
}
}
JSValue nativeGetCurrentStack() { return state().get("currentStack"); }
void nativeSetCurrentStack(const JSValue& stack) { state().set("currentStack", stack); }
JSValue nativeStackNodeConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isConstructible()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "StackNode");
    const auto base = nativeNodeConstructor();
    auto constructor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        auto self = captures[0].construct({}, newTarget);
        self.set("nodes", JSValue::array()); self.set("outputNode", nullptr);
        const auto parent = arg(args, 0); self.set("parent", parent.isUndefined() ? JSValue(nullptr) : parent);
        self.set("_currentCond", nullptr); self.set("_expressionNode", nullptr); self.set("_currentNode", nullptr);
        self.set("_nodeDataLibrary", createNativeMap()); self.set("isStackNode", true); return self;
    }, {base}, "StackNode", 0);
    constructor.setPrototype(base); auto prototype = constructor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type; type.getter = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return "StackNode"; }, {}, "get type", 0); type.configurable = true; constructor.defineProperty("type", type);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
        {"isCacheable", 0}, {"getElementType", 1}, {"generateNodeType", 1}, {"getMemberType", 2}, {"addToStack", 1}, {"addToStackBefore", 1}, {"If", 2}, {"ElseIf", 2}, {"Else", 1}, {"Switch", 1}, {"Case", 0}, {"Default", 1}, {"setup", 1}, {"build", 1}}) {
        JSPropertyDescriptor descriptor; descriptor.value = JSValue::function(method, {name}, name, arity); descriptor.writable = descriptor.configurable = true; prototype.defineProperty(name, descriptor);
    }
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
StackNode::StackNode(const JSValue& parent) : object_(constructor().construct({parent})) {}
JSValue StackNode::addToStack(const JSValue& node, const JSValue& index) const { return invoke(object_, "addToStack")({node, index}); }
JSValue StackNode::addToStackBefore(const JSValue& node) const { return invoke(object_, "addToStackBefore")({node}); }
JSValue StackNode::build(const JSValue& builder, const Args& params) const { Args args{builder}; args.insert(args.end(), params.begin(), params.end()); return invoke(object_, "build")(args); }
