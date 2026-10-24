#include "nodes/core/ParameterNode.h"

#include <string>

std::string ParameterNode::memberType(bool hasStruct, const std::string& structAnswer) {
    // The struct answers when it is there; a float otherwise, after an error has been reported.
    if (hasStruct) return structAnswer;
    return kMissingStructureMemberType;
}

std::string ParameterNode::missingStructureMessage(const std::string& name,
                                                   const std::string& type) {
    return "TSL: Member \"" + name + "\" not found in struct \"" + type + "\".";
}

std::string ParameterNode::hash(std::uint64_t id) {
    // The identifier as text rather than as a number.
    return std::to_string(id);
}

std::string ParameterNode::generate(bool hasName, const std::string& name) {
    // No check that a name was given, so an absent one produces whatever an absent one renders
    // as rather than anything the node supplies.
    if (hasName) return name;
    return "null";
}

#include "nodes/core/PropertyNode.h"
#include "nodes/core/StackTrace.h"
#include "core/JSModuleObject.h"
#include "utils.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i, const JSValue& fallback = {}) { return i < args.size() && !args[i].isUndefined() ? args[i] : fallback; }
struct Invocation { JSValue object, method; JSValue operator()(const Args& args = {}) const { return method.call(object, args); } };
Invocation invoke(const JSValue& object, const char* key) { return {object, object.get(key)}; }
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    const auto name = captures[0].string(); const auto self = receiver;
    if (name == "getHash") { const auto id = self.get("id"); return id.isSymbol() ? JSValue("Symbol(" + id.symbolDescription().value_or("") + ")") : JSValue(id.toString()); }
    if (name == "generate") return self.get("name");
    if (name == "getMemberType") {
        const auto builder = arg(args, 0), member = arg(args, 1), type = invoke(self, "getNodeType")({builder});
        const auto structure = invoke(builder, "getStructTypeNode")({type});
        if (!structure.isNull()) return invoke(structure, "getMemberType")({builder, member});
        const auto memberText = member.toString(); const auto typeText = type.toString();
        ThreeUtils::error({"TSL: Member \"" + memberText + "\" not found in struct \"" + typeText + "\".", nativeStackTraceConstructor().construct()}); return "float";
    }
    throw std::logic_error("Unknown ParameterNode method");
}
}
JSValue nativeParameterNodeConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto ctor = weak.lock(); ctor && ctor->isConstructible()) return *ctor;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "ParameterNode"); auto holder = JSValue::object(nullptr); const auto base = nativePropertyNodeConstructor();
    auto ctor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        auto self = captures[0].get("constructor").getPrototype().construct({arg(args, 0), arg(args, 1, nullptr)}, newTarget); self.set("isParameterNode", true); return self;
    }, {holder}, "ParameterNode", 1);
    holder.set("constructor", ctor); ctor.setPrototype(base); auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type; type.getter = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return "ParameterNode"; }, {}, "get type", 0); type.configurable = true; ctor.defineProperty("type", type);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"getMemberType", 2}, {"getHash", 0}, {"generate", 0}}) {
        JSPropertyDescriptor d; d.value = JSValue::function(method, {name}, name, arity); d.writable = d.configurable = true; prototype.defineProperty(name, d);
    }
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
ParameterNode::ParameterNode(const JSValue& type, const JSValue& name) : object_(constructor().construct({type, name})) {}
