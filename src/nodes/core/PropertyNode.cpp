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
#include <algorithm>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i, const JSValue& fallback = {}) { return i < args.size() && !args[i].isUndefined() ? args[i] : fallback; }
struct Invocation { JSValue object, method; JSValue operator()(const Args& args = {}) const { return method.call(object, args); } };
Invocation invoke(const JSValue& object, const char* name) { return {object, object.get(name)}; }
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(); const auto b = right.toPrimitive();
    return a.type() == JSValue::Type::String || b.type() == JSValue::Type::String ? JSValue(a.toString() + b.toString()) : JSValue(a.toNumber() + b.toNumber());
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
        return nativeNodeUtilsBinding("hashString").call({}, {text});
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
        self.set("name", arg(args, 1, nullptr)); self.set("varying", arg(args, 2, false)); self.set("placeholderNode", captures[1].call({}, {arg(args, 3, nullptr)}));
        self.set("isPropertyNode", true); self.set("global", true); return self;
    }, {holder, nativeTSLCoreBinding("nodeObject")}, "PropertyNode", 1);
    holder.set("constructor", ctor); ctor.setPrototype(base); auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type; type.getter = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return "PropertyNode"; }, {}, "get type", 0); type.configurable = true; ctor.defineProperty("type", type);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"isCacheable", 0}, {"getNodeType", 1}, {"customCacheKey", 0}, {"getHash", 1}, {"generate", 1}}) {
        JSPropertyDescriptor d; d.value = JSValue::function(method, {name, prototype}, name, arity); d.writable = d.configurable = true; prototype.defineProperty(name, d);
    }
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
PropertyNode::PropertyNode(const JSValue& type, const JSValue& name, const JSValue& varying, const JSValue& placeholderNode) : object_(constructor().construct({type, name, varying, placeholderNode})) {}

namespace { JSWeakValue& propertyLexicalBindings(){static thread_local JSWeakValue weak;return weak;} }
JSValue nativePropertyNodeModule() {
    static thread_local JSWeakValue weak;
    if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"PropertyNode exports");
    const auto tsl=nativeTSLCore(),constructor=nativePropertyNodeConstructor();auto exports=JSValue::object();
    const auto own=[&](const char* name,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;exports.defineProperty(name,d);};
    own("default",constructor);
    for(const auto& [name,varying]:std::initializer_list<std::pair<const char*,bool>>{{"property",false},{"varyingProperty",true}})
        own(name,JSValue::function([](const JSValue&,const Args& args,const Args& captures)->JSValue {
            return captures[0].construct({arg(args,0),arg(args,1),captures[1],arg(args,2,nullptr)});
        },{constructor,varying},name,2));
    const auto immutable=nativeTSLCoreBinding("nodeImmutable");
    own("diffuseColor",immutable.call({},{constructor,"vec4", "DiffuseColor"}));
    own("diffuseContribution",immutable.call({},{constructor,"vec3", "DiffuseContribution"}));
    own("diffuseRoughness",immutable.call({},{constructor,"float", "DiffuseRoughness"}));
    own("emissive",immutable.call({},{constructor,"vec3", "EmissiveColor"}));
    own("roughness",immutable.call({},{constructor,"float", "Roughness"}));
    own("metalness",immutable.call({},{constructor,"float", "Metalness"}));
    own("clearcoat",immutable.call({},{constructor,"float", "Clearcoat"}));
    own("clearcoatRoughness",immutable.call({},{constructor,"float", "ClearcoatRoughness"}));
    own("sheen",immutable.call({},{constructor,"vec3", "Sheen"}));
    own("sheenRoughness",immutable.call({},{constructor,"float", "SheenRoughness"}));
    own("iridescence",immutable.call({},{constructor,"float", "Iridescence"}));
    own("iridescenceIOR",immutable.call({},{constructor,"float", "IridescenceIOR"}));
    own("iridescenceThickness",immutable.call({},{constructor,"float", "IridescenceThickness"}));
    own("alphaT",immutable.call({},{constructor,"float", "AlphaT"}));
    own("anisotropy",immutable.call({},{constructor,"float", "Anisotropy"}));
    own("anisotropyT",immutable.call({},{constructor,"vec3", "AnisotropyT"}));
    own("anisotropyB",immutable.call({},{constructor,"vec3", "AnisotropyB"}));
    own("specularColor",immutable.call({},{constructor,"color", "SpecularColor"}));
    own("specularColorBlended",immutable.call({},{constructor,"color", "SpecularColorBlended"}));
    own("specularF90",immutable.call({},{constructor,"float", "SpecularF90"}));
    own("shininess",immutable.call({},{constructor,"float", "Shininess"}));
    own("output",immutable.call({},{constructor,"output", "Output"}));
    own("dashSize",immutable.call({},{constructor,"float", "dashSize"}));
    own("gapSize",immutable.call({},{constructor,"float", "gapSize"}));
    own("pointWidth",immutable.call({},{constructor,"float", "pointWidth"}));
    own("ior",immutable.call({},{constructor,"float", "IOR"}));
    own("transmission",immutable.call({},{constructor,"float", "Transmission"}));
    own("thickness",immutable.call({},{constructor,"float", "Thickness"}));
    own("attenuationDistance",immutable.call({},{constructor,"float", "AttenuationDistance"}));
    own("attenuationColor",immutable.call({},{constructor,"color", "AttenuationColor"}));
    own("dispersion",immutable.call({},{constructor,"float", "Dispersion"}));
    own("retroreflectivity",immutable.call({},{constructor,"float", "Retroreflectivity"}));
    own("ambientOcclusion",immutable.call({},{constructor,"float", "AmbientOcclusion", false, 1}));
    auto names=exports.ownPropertyNames();std::sort(names.begin(),names.end());auto module=JSValue::object();
    for(const auto& name:names){JSPropertyDescriptor d;d.value=exports.get(name);d.writable=d.enumerable=d.configurable=true;module.defineProperty(name,d);}
    exports.retainForHeapLifetime();propertyLexicalBindings()=JSWeakValue(exports);
    module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativePropertyNodeBinding(const std::string& name){const auto keeper=nativePropertyNodeModule();return propertyLexicalBindings().lock()->get(name);}
