#include "nodes/math/ConditionalNode.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeError.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/tsl/TSLCore.h"
#include "core/JSPromise.h"
#include "core/JSModuleObject.h"
#include "core/JSStringEncoding.h"
#include "utils.h"
#include <cmath>
#include <limits>

namespace {
using Args = std::vector<JSValue>;
JSWeakValue& lexicalBindings() { static thread_local JSWeakValue weak; return weak; }
JSValue arg(const Args& a, unsigned i, const JSValue& fallback = {}) { return i < a.size() && !a[i].isUndefined() ? a[i] : fallback; }
struct Invocation { JSValue object, method; JSValue operator()(const Args& a = {}) const { return method.call(object, a); } };
Invocation invoke(const JSValue& o, const char* k) { return {o, o.get(k)}; }
JSValue plus(const JSValue& a, const JSValue& b) {
    const auto x = a.toPrimitive(); const auto y = b.toPrimitive();
    if (x.type() == JSValue::Type::String || y.type() == JSValue::Type::String) return x.toString() + y.toString();
    return x.toNumber() + y.toNumber();
}
bool greater(const JSValue& a, const JSValue& b) {
    const auto x = a.toPrimitive(JSValue::PreferredType::Number); const auto y = b.toPrimitive(JSValue::PreferredType::Number);
    if (x.type() == JSValue::Type::String && y.type() == JSValue::Type::String) return jsUTF16(x.string()) > jsUTF16(y.string());
    const auto left = x.toNumber(); const auto right = y.toNumber(); return left > right;
}
JSValue max(const JSValue& a, const JSValue& b) {
    const auto x = a.toNumber(); const auto y = b.toNumber();
    if (std::isnan(x) || std::isnan(y)) return std::numeric_limits<double>::quiet_NaN();
    if (x == 0 && y == 0) return std::signbit(x) && std::signbit(y) ? -0.0 : 0.0;
    return x > y ? x : y;
}
JSValue record(const char* key, const JSValue& value) { auto o = JSValue::object(); o.set(key, value); return o; }
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string(); auto builder = arg(args, 0);
    if (name == "isCacheable") return false;
    if (name == "generateNodeType") {
        const auto properties = invoke(builder, "getNodeProperties")({self});
        const auto condition = properties.get("condNode"); const auto yes = properties.get("ifNode"); const auto no = properties.get("elseNode");
        if (yes.isUndefined()) { invoke(builder, "flowBuildStage")({self, "setup"}); return invoke(self, "getNodeType")({builder}); }
        auto type = invoke(yes, "getNodeType")({builder});
        if (!no.isNull()) {
            const auto elseType = invoke(no, "getNodeType")({builder});
            const auto elseLength = invoke(builder, "getTypeLength")({elseType}); const auto ifLength = invoke(builder, "getTypeLength")({type});
            if (greater(elseLength, ifLength)) type = elseType;
        }
        const auto lengthMethod = invoke(builder, "getTypeLength"); const auto conditionType = invoke(condition, "getNodeType")({builder});
        const auto conditionLength = lengthMethod({conditionType});
        if (greater(conditionLength, 1) && !invoke(builder, "isReference")({type}).toBoolean()) {
            const auto typeLength = invoke(builder, "getTypeLength")({type});
            bool promote = typeLength.strictEquals(1);
            if (!promote) { const auto isVector = invoke(builder, "isVector"); const auto vectorType = invoke(builder, "getVectorType")({type}); promote = isVector({vectorType}).toBoolean(); }
            if (promote) {
                const auto getType = invoke(builder, "getTypeFromLength"); const auto length = invoke(builder, "getTypeLength")({type}); const auto largest = max(conditionLength, length);
                const auto component = invoke(builder, "getComponentType")({type}); type = getType({largest, component});
            }
        }
        return type;
    }
    if (name == "setup") {
        const auto condition = self.get("condNode"); const auto yes = self.get("ifNode"); const auto no = self.get("elseNode");
        const auto uniform = builder.get("context").get("uniformFlow"); auto properties = invoke(builder, "getNodeProperties")({self});
        properties.set("condNode", condition);
        properties.set("ifNode", uniform.toBoolean() ? yes : invoke(yes, "context")({record("nodeBlock", yes)}));
        properties.set("elseNode", no.toBoolean() ? (uniform.toBoolean() ? no : invoke(no, "context")({record("nodeBlock", no)})) : JSValue(nullptr));
        return {};
    }
    if (name == "generate") {
        const auto output = arg(args, 1); const auto type = invoke(self, "getNodeType")({builder}); auto data = invoke(builder, "getDataFromNode")({self});
        if (!data.get("propertyName").isUndefined()) { const auto format = invoke(builder, "format"); return format({data.get("propertyName"), type, output}); }
        const auto properties = invoke(builder, "getNodeProperties")({self});
        const auto condition = properties.get("condNode"); const auto yes = properties.get("ifNode"); const auto no = properties.get("elseNode");
        const auto function = builder.get("currentFunctionNode"); const bool needsOutput = !output.strictEquals("void");
        JSValue property = "";
        if (needsOutput) { const auto node = captures[1].call({}, {type}); property = invoke(node, "build")({builder}); }
        data.set("propertyName", property);
        const auto conditionType = invoke(condition, "getNodeType")({builder}); const auto length = invoke(builder, "getTypeLength")({conditionType});
        if (greater(length, 1)) {
            const auto vectorType = invoke(builder, "getVectorType")({type});
            if (invoke(builder, "isReference")({type}).toBoolean() || !invoke(builder, "isVector")({vectorType}).toBoolean()) {
                const auto conditionText = conditionType.toString(); const auto typeText = type.toString();
                const auto message = "TSL: select() with a vector condition (\"" + conditionText + "\") requires scalar or vector values, received \"" + typeText + "\".";
                throw JSThrownValue(nativeNodeErrorConstructor().construct({message, self.get("stackTrace")}));
            }
            const auto elseSnippet = !no.isNull() ? invoke(no, "build")({builder, type}) : invoke(builder, "generateConst")({type});
            const auto boolType = invoke(builder, "changeComponentType")({type, "bool"});
            const auto conditionSnippet = invoke(condition, "build")({builder, boolType}); const auto ifSnippet = invoke(yes, "build")({builder, type});
            const auto snippet = invoke(builder, "getVectorSelect")({conditionSnippet, ifSnippet, elseSnippet, type});
            if (!needsOutput) return "";
            const auto add = invoke(builder, "addFlowCode"); auto text = std::string("\n") + builder.get("tab").toString(); text += property.toString(); text += " = "; text += snippet.toString(); text += ";\n\n"; add({text});
            return invoke(builder, "format")({property, type, output});
        }
        const auto conditionSnippet = invoke(condition, "build")({builder, "bool"}); const auto uniform = builder.get("context").get("uniformFlow");
        if (uniform.toBoolean() && !no.isNull()) {
            const auto ifSnippet = invoke(yes, "build")({builder, type}); const auto elseSnippet = invoke(no, "build")({builder, type});
            const auto snippet = invoke(builder, "getTernary")({conditionSnippet, ifSnippet, elseSnippet}); return invoke(builder, "format")({snippet, type, output});
        }
        const auto add = invoke(builder, "addFlowCode"); auto opening = std::string("\n") + builder.get("tab").toString(); opening += "if ( "; opening += conditionSnippet.toString(); opening += " ) {\n\n";
        invoke(add({opening}), "addFlowTab")();
        const auto block = builder.get("flowBlock"); builder.set("flowBlock", record("parent", block));
        auto ifSnippet = invoke(yes, "build")({builder, type}); builder.set("flowBlock", block);
        if (ifSnippet.toBoolean()) {
            if (needsOutput) ifSnippet = plus(plus(plus(property, " = "), ifSnippet), ";");
            else {
                ifSnippet = plus(plus("return ", ifSnippet), ";");
                if (function.isNull()) { ThreeUtils::warn({"TSL: Return statement used in an inline 'Fn()'. Define a layout struct to allow return values.", self.get("stackTrace")}); ifSnippet = plus("// ", ifSnippet); }
            }
        }
        const auto afterIf = invoke(invoke(builder, "removeFlowTab")(), "addFlowCode");
        auto closing = plus(builder.get("tab"), "\t"); closing = plus(closing, ifSnippet); closing = plus(closing, "\n\n"); closing = plus(closing, builder.get("tab")); closing = plus(closing, "}"); afterIf({closing});
        if (!no.isNull()) {
            invoke(invoke(builder, "addFlowCode")({" else {\n\n"}), "addFlowTab")(); builder.set("flowBlock", record("parent", block));
            auto elseSnippet = invoke(no, "build")({builder, type}); builder.set("flowBlock", block);
            if (elseSnippet.toBoolean()) {
                if (needsOutput) elseSnippet = plus(plus(plus(property, " = "), elseSnippet), ";");
                else {
                    elseSnippet = plus(plus("return ", elseSnippet), ";");
                    if (function.isNull()) { ThreeUtils::warn({"TSL: Return statement used in an inline 'Fn()'. Define a layout struct to allow return values.", self.get("stackTrace")}); elseSnippet = plus("// ", elseSnippet); }
                }
            }
            const auto afterElse = invoke(invoke(builder, "removeFlowTab")(), "addFlowCode");
            auto closing = plus(builder.get("tab"), "\t"); closing = plus(closing, elseSnippet); closing = plus(closing, "\n\n"); closing = plus(closing, builder.get("tab")); closing = plus(closing, "}\n\n"); afterElse({closing});
        } else invoke(builder, "addFlowCode")({"\n\n"});
        return invoke(builder, "format")({property, type, output});
    }
    throw std::logic_error("Unknown ConditionalNode method");
}
}
JSValue nativeConditionalNodeConstructor() {
    static thread_local JSWeakValue weak; if (const auto value = weak.lock(); value && value->isConstructible()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "ConditionalNode");
    const auto base = nativeNodeConstructor(); const auto property = nativePropertyNodeBinding("property"); auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor([](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
        auto self = captures[0].get("constructor").getPrototype().construct({}, target); self.set("condNode", arg(args, 0)); self.set("ifNode", arg(args, 1)); self.set("elseNode", arg(args, 2, nullptr)); return self;
    }, {holder}, "ConditionalNode", 2);
    holder.set("constructor", ctor); ctor.setPrototype(base); auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type; type.getter = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return "ConditionalNode"; }, {}, "get type", 0); type.configurable = true; ctor.defineProperty("type", type);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"isCacheable",0},{"generateNodeType",1},{"setup",1},{"generate",2}}) {
        JSPropertyDescriptor d; d.value = JSValue::function(method, {name, property}, name, arity); d.writable = d.configurable = true; prototype.defineProperty(name, d);
    }
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
JSValue nativeConditionalNodeModule() {
    static thread_local JSWeakValue weak; if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "ConditionalNode exports");
    const auto tsl = nativeTSLCore(); const auto ctor = nativeConditionalNodeConstructor();
    const auto proxy = nativeTSLCoreBinding("nodeProxy").call({}, {ctor}); const auto select = invoke(proxy, "setParameterLength")({2,3});
    auto bindings = JSValue::object(); bindings.set("default", ctor); bindings.set("select", select); nativeTSLCoreBinding("addMethodChaining").call({}, {"select", select});
    auto module = JSValue::object(); module.set("default", ctor); module.set("select", select);
    bindings.retainForHeapLifetime(); lexicalBindings() = JSWeakValue(bindings);
    module.retainForHeapLifetime(); weak = JSWeakValue(module); return module;
}
JSValue nativeConditionalNodeBinding(const std::string& name) { const auto keeper = nativeConditionalNodeModule(); return lexicalBindings().lock()->get(name); }
ConditionalNode::ConditionalNode(const JSValue& condition, const JSValue& yes, const JSValue& no) : object_(constructor().construct({condition,yes,no})) {}
JSValue ConditionalNode::generateNodeType(const JSValue& builder) const { return invoke(object_, "generateNodeType")({builder}); }
JSValue ConditionalNode::setup(const JSValue& builder) const { return invoke(object_, "setup")({builder}); }
JSValue ConditionalNode::generate(const JSValue& builder, const JSValue& output) const { return invoke(object_, "generate")({builder,output}); }
