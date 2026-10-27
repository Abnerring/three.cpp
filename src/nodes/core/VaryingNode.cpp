#include "nodes/core/VaryingNode.h"

bool VaryingNode::needsInterpolationAssigned(const std::string& shaderStage) {
    // The value assigned is whether the stage is the fragment one.
    return shaderStage == "fragment";
}

bool VaryingNode::overwritesExistingFlag() {
    // The short circuit means an existing truthy value is left alone.
    return false;
}

VaryingNode::Emission VaryingNode::emissionFor(const std::string& shaderStage) {
    // The vertex stage guards on the flow block; the others guard on nothing.
    if (shaderStage == "vertex") return Emission::PerBlock;
    return Emission::OnceInTotal;
}

bool VaryingNode::setupAndAnalyzeAreTheSame() {
    return true;
}

std::string VaryingNode::hash(bool hasName, const std::string& name,
                              const std::string& ordinaryHash) {
    // The explicit name when there is one, the ordinary hash otherwise.
    if (hasName) return name;
    return ordinaryHash;
}

bool VaryingNode::isCacheable() {
    return false;
}

#include "nodes/core/Node.h"
#include "nodes/core/SubBuildNode.h"
#include "nodes/core/constants.h"
#include "nodes/tsl/TSLCore.h"
#include "core/JSModuleObject.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i,const JSValue& fallback = {}) { return i < args.size() && !args[i].isUndefined() ? args[i] : fallback; }
struct Invocation { JSValue object, method; JSValue operator()(const Args& args = {}) const { return method.call(object,args); } };
Invocation invoke(const JSValue& object,const char* key) { return {object,object.get(key)}; }
JSWeakValue& lexicalBindings() { static thread_local JSWeakValue weak; return weak; }
JSValue method(const JSValue& self,const Args& args,const Args& captures) {
    const auto name = captures[0].string(); const auto builder = arg(args,0);
    if (name == "isCacheable") return false;
    if (name == "setInterpolation") { auto node = self; node.set("interpolationType",arg(args,0)); node.set("interpolationSampling",arg(args,1,nullptr)); return self; }
    if (name == "getHash") { const auto value = self.get("name"); return value.toBoolean() ? value : captures[1].getPrototype().get("getHash",self).call(self,{builder}); }
    if (name == "generateNodeType") return invoke(self.get("node"),"getNodeType")({builder});
    if (name == "setupVarying") {
        auto properties = invoke(builder,"getNodeProperties")({self}); auto varying = properties.get("varying");
        if (varying.isUndefined()) {
            const auto name = self.get("name"), type = invoke(self,"getNodeType")({builder}); const auto interpolationType = self.get("interpolationType"), interpolationSampling = self.get("interpolationSampling");
            varying = invoke(builder,"getVaryingFromNode")({self,name,type,interpolationType,interpolationSampling}); properties.set("varying",varying);
            const auto node = captures[2].call({}, {self.get("node"),"VERTEX"}); properties.set("node",node);
        }
        if (!varying.get("needsInterpolation").toBoolean()) varying.set("needsInterpolation",builder.get("shaderStage").strictEquals("fragment"));
        return varying;
    }
    if (name == "setup" || name == "analyze") {
        invoke(self,"setupVarying")({builder}); const auto flow = invoke(builder,"flowNodeFromShaderStage"); const auto vertex = captures[3].get("VERTEX"); flow({vertex,self.get("node")}); return {};
    }
    if (name == "generate") {
        const auto propertyMethod = invoke(builder,"getSubBuildProperty"); const auto propertyKey = propertyMethod({"property",builder.get("currentStack")}); auto properties = invoke(builder,"getNodeProperties")({self});
        const auto varying = invoke(self,"setupVarying")({builder}); const auto stage = builder.get("shaderStage"); const auto vertex = captures[3].get("VERTEX"); const auto flowBlock = stage.strictEquals(vertex) ? builder.get("flowBlock") : JSValue(nullptr);
        if (!properties.get(propertyKey).strictEquals(flowBlock)) {
            const auto type = invoke(self,"getNodeType")({builder}); const auto getPropertyName = invoke(builder,"getPropertyName"); const auto propertyName = getPropertyName({varying,captures[3].get("VERTEX")});
            if (builder.get("shaderStage").strictEquals(captures[3].get("VERTEX"))) {
                const auto snippet = invoke(properties.get("node"),"build")({builder,type}); const auto line = invoke(builder,"addLineFlowCode"); const auto propertyText = propertyName.toString(); const auto snippetText = snippet.toString(); line({propertyText+" = "+snippetText,self});
            } else { const auto flow = invoke(builder,"flowNodeFromShaderStage"); flow({captures[3].get("VERTEX"),properties.get("node"),type,propertyName}); }
            properties.set(propertyKey,flowBlock);
        }
        return invoke(builder,"getPropertyName")({varying});
    }
    throw std::logic_error("Unknown VaryingNode method");
}
}
JSValue nativeVaryingNodeConstructor() {
    static thread_local JSWeakValue weak; if (const auto value = weak.lock(); value && value->isConstructible()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing,"VaryingNode");
    const auto base = nativeNodeConstructor(), subBuild = nativeSubBuildNodeBinding("subBuild"), stages = nativeNodeConstantsBinding("NodeShaderStage"); auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor([](const JSValue& target,const Args& args,const Args& captures) -> JSValue {
        auto self = captures[0].get("constructor").getPrototype().construct({},target); self.set("node",captures[1].call({}, {arg(args,0),"VERTEX"})); self.set("name",arg(args,1,nullptr));
        self.set("isVaryingNode",true); self.set("interpolationType",nullptr); self.set("interpolationSampling",nullptr); self.set("global",true); return self;
    },{holder,subBuild},"VaryingNode",1);
    holder.set("constructor",ctor); ctor.setPrototype(base); auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type; type.getter = JSValue::function([](const JSValue&,const Args&,const Args&) -> JSValue { return "VaryingNode"; },{},"get type",0); type.configurable = true; ctor.defineProperty("type",type);
    for (const auto& [name,arity] : std::initializer_list<std::pair<const char*,unsigned>>{{"isCacheable",0},{"setInterpolation",1},{"getHash",1},{"generateNodeType",1},{"setupVarying",1},{"setup",1},{"analyze",1},{"generate",1}}) {
        JSPropertyDescriptor d; d.value = JSValue::function(method,{name,prototype,subBuild,stages},name,arity); d.writable = d.configurable = true; prototype.defineProperty(name,d);
    }
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
JSValue nativeVaryingNodeModule() {
    static thread_local JSWeakValue weak; if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing,"VaryingNode exports");
    const auto ctor = nativeVaryingNodeConstructor(), proxy = nativeTSLCoreBinding("nodeProxy").call({}, {ctor}); const auto varying = invoke(proxy,"setParameterLength")({1,2});
    const auto vertexStage = JSValue::function([](const JSValue&,const Args& args,const Args& captures) -> JSValue { return captures[0].call({}, {arg(args,0)}); },{varying},"vertexStage",1);
    auto bindings = JSValue::object(); bindings.set("default",ctor); bindings.set("varying",varying); bindings.set("vertexStage",vertexStage);
    const auto add = nativeTSLCoreBinding("addMethodChaining"); add.call({}, {"toVarying",varying}); add.call({}, {"toVertexStage",vertexStage});
    auto exports = JSValue::object(); exports.set("default",ctor); exports.set("varying",varying); exports.set("vertexStage",vertexStage);
    bindings.retainForHeapLifetime(); lexicalBindings() = JSWeakValue(bindings); exports.retainForHeapLifetime(); weak = JSWeakValue(exports); return exports;
}
JSValue nativeVaryingNodeBinding(const std::string& name) { const auto keeper = nativeVaryingNodeModule(); return lexicalBindings().lock()->get(name); }
VaryingNode::VaryingNode(const JSValue& node,const JSValue& name) : object_(constructor().construct({node,name})) {}
JSValue VaryingNode::setInterpolation(const JSValue& type,const JSValue& sampling) const { return invoke(object_,"setInterpolation")({type,sampling}); }
JSValue VaryingNode::getHash(const JSValue& builder) const { return invoke(object_,"getHash")({builder}); }
JSValue VaryingNode::generateNodeType(const JSValue& builder) const { return invoke(object_,"generateNodeType")({builder}); }
JSValue VaryingNode::setupVarying(const JSValue& builder) const { return invoke(object_,"setupVarying")({builder}); }
JSValue VaryingNode::setup(const JSValue& builder) const { return invoke(object_,"setup")({builder}); }
JSValue VaryingNode::analyze(const JSValue& builder) const { return invoke(object_,"analyze")({builder}); }
JSValue VaryingNode::generate(const JSValue& builder) const { return invoke(object_,"generate")({builder}); }
