#include "nodes/core/PropertyNode.h"

std::string PropertyNode::nodeType(const std::string& declaredType,
                                   const std::string& builderOutputType) {
    // The one name that is a request to look the type up rather than a type.
    if (declaredType == kOutputTypeName) return builderOutputType;
    return declaredType;
}

std::string PropertyNode::cacheKeyText(const std::string& type, bool hasName,
                                       const std::string& name, bool varying) {
    // Type, name, flag; the flag as a digit and an absent name as nothing.
    std::string key = type;
    key += ':';
    if (hasName) key += name;
    key += ':';
    key += varying ? '1' : '0';
    return key;
}

bool PropertyNode::isVarying(bool varying) {
    return varying;
}

bool PropertyNode::interpolationSetUnconditionally() {
    // The source assigns true without asking whether it is already set.
    return true;
}

bool PropertyNode::writesPlaceholder(bool hasPlaceholder, bool hasWriteUsage) {
    // Only with a placeholder and only when nothing writes to the property.
    if (!hasPlaceholder) return false;
    return !hasWriteUsage;
}

bool PropertyNode::isCacheable() {
    return false;
}

#include "nodes/core/Node.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/tsl/TSLCore.h"
#include "core/JSModuleObject.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i, const JSValue& fallback = {}) { return i < args.size() && !args[i].isUndefined() ? args[i] : fallback; }
struct Invocation { JSValue object, method; JSValue operator()(const Args& args = {}) const { return method.call(object, args); } };
Invocation invoke(const JSValue& object, const char* name) { return {object, object.get(name)}; }
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(); const auto b = right.toPrimitive();
    return a.type() == JSValue::Type::String || b.type() == JSValue::Type::String ? JSValue(a.toString() + b.toString()) : JSValue(a.toNumber() + b.toNumber());
}
JSValue placeholder(const JSValue& value) {
    return nativeTSLCore().get("nodeObject").call({}, {value});
}
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string(); const auto builder = arg(args, 0);
    if (name == "isCacheable") return false;
    if (name == "getNodeType") {
        const auto type = captures[1].getPrototype().get("getNodeType", self).call(self, {builder});
        return type.strictEquals("output") ? invoke(builder, "getOutputType")() : type;
    }
    if (name == "customCacheKey") {
        auto text = add(self.get("type"), ":"); const auto propertyName = self.get("name"); text = add(text, propertyName.toBoolean() ? propertyName : JSValue(""));
        text = add(text, ":"); text = add(text, self.get("varying").toBoolean() ? JSValue("1") : JSValue("0"));
        return nativeNodeUtils().get("hashString").call({}, {text});
    }
    if (name == "getHash") { const auto propertyName = self.get("name"); return propertyName.toBoolean() ? propertyName : captures[1].getPrototype().get("getHash", self).call(self, {builder}); }
    if (name == "generate") {
        JSValue variable;
        if (self.get("varying").strictEquals(true)) { variable = invoke(builder, "getVaryingFromNode")({self, self.get("name")}); variable.set("needsInterpolation", true); }
        else {
            variable = invoke(builder, "getVarFromNode")({self, self.get("name")});
            if (!self.get("placeholderNode").isNull() && invoke(builder, "hasWriteUsage")({self}).strictEquals(false)) {
                const auto snippet = invoke(self.get("placeholderNode"), "build")({builder, invoke(self, "getNodeType")({builder})});
                const auto callback = invoke(builder, "addLineFlowCode"); const auto propertyName = invoke(builder, "getPropertyName")({variable});
                const auto propertyText = propertyName.toString(); const auto snippetText = snippet.toString();
                callback({propertyText + " = " + snippetText, self});
            }
        }
        return invoke(builder, "getPropertyName")({variable});
    }
    throw std::logic_error("Unknown PropertyNode method");
}
}
JSValue nativePropertyNodeConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto ctor = weak.lock(); ctor && ctor->isConstructible()) return *ctor;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "PropertyNode"); auto holder = JSValue::object(nullptr); const auto base = nativeNodeConstructor();
    auto ctor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        auto self = captures[0].get("constructor").getPrototype().construct({arg(args, 0)}, newTarget);
        self.set("name", arg(args, 1, nullptr)); self.set("varying", arg(args, 2, false)); self.set("placeholderNode", placeholder(arg(args, 3, nullptr)));
        self.set("isPropertyNode", true); self.set("global", true); return self;
    }, {holder}, "PropertyNode", 1);
    holder.set("constructor", ctor); ctor.setPrototype(base); auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type; type.getter = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return "PropertyNode"; }, {}, "get type", 0); type.configurable = true; ctor.defineProperty("type", type);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"isCacheable", 0}, {"getNodeType", 1}, {"customCacheKey", 0}, {"getHash", 1}, {"generate", 1}}) {
        JSPropertyDescriptor d; d.value = JSValue::function(method, {name, prototype}, name, arity); d.writable = d.configurable = true; prototype.defineProperty(name, d);
    }
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
PropertyNode::PropertyNode(const JSValue& type, const JSValue& name, const JSValue& varying, const JSValue& placeholderNode) : object_(constructor().construct({type, name, varying, placeholderNode})) {}
