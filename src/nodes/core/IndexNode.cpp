#include "nodes/core/IndexNode.h"

std::vector<std::string> IndexNode::scopes() {
    return {"vertex", "instance", "subgroup", "invocationLocal", "invocationSubgroup", "draw"};
}

std::string IndexNode::builderCall(const std::string& scope) {
    if (scope == "vertex") return "getVertexIndex";
    if (scope == "instance") return "getInstanceIndex";
    if (scope == "draw") return "getDrawIndex";
    if (scope == "invocationLocal") return "getInvocationLocalIndex";
    if (scope == "invocationSubgroup") return "getInvocationSubgroupIndex";
    if (scope == "subgroup") return "getSubgroupIndex";
    // No default: the source throws here.
    return std::string();
}

bool IndexNode::isKnown(const std::string& scope) {
    return !builderCall(scope).empty();
}

std::string IndexNode::unknownScopeMessage(const std::string& scope) {
    return "THREE.IndexNode: Unknown scope: " + scope;
}

bool IndexNode::readsDirectly(const std::string& shaderStage) {
    // Two stages, not one: the compute stage reads it as the vertex stage does.
    return shaderStage == "vertex" || shaderStage == "compute";
}

#include "nodes/core/VaryingNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include <algorithm>
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& indexBindings(){static thread_local JSWeakValue weak;return weak;}
JSValue indexMethod(const JSValue& self,const Args& args,const Args& captures){
    const auto name=captures[0],bindings=captures[2];if(name.strictEquals("isCacheable"))return false;
    const auto builder=arg(args,0),type=invoke(self,"getNodeType")({builder}),scope=self.get("scope"),ctor=bindings.get("default");JSValue property;bool found=false;
    for(const auto& [constant,method]:std::initializer_list<std::pair<const char*,const char*>>{{"VERTEX","getVertexIndex"},{"INSTANCE","getInstanceIndex"},{"DRAW","getDrawIndex"},{"INVOCATION_LOCAL","getInvocationLocalIndex"},{"INVOCATION_SUBGROUP","getInvocationSubgroupIndex"},{"SUBGROUP","getSubgroupIndex"}}){
        if(scope.strictEquals(ctor.get(constant))){property=invoke(builder,method)();found=true;break;}
    }
    if(!found)throw std::runtime_error(add("THREE.IndexNode: Unknown scope: ",scope).toString());
    if(builder.get("shaderStage").strictEquals("vertex")||builder.get("shaderStage").strictEquals("compute"))return property;
    const auto varying=bindings.get("varying").call({}, {self});return invoke(varying,"build")({builder,type});
}
}
JSValue nativeIndexNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"IndexNode module");const auto base=nativeNodeConstructor(),tsl=nativeTSLCore(),varying=nativeVaryingNodeBinding("varying");auto bindings=JSValue::object(nullptr);
    own(bindings,"varying",varying);const auto ctor=makeConstructor("IndexNode",1,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{auto self=superConstruct(target,captures,{"uint"});self.set("scope",arg(args,0));self.set("isIndexNode",true);return self;},{{"isCacheable",0},{"generate",1}},indexMethod,{bindings});own(bindings,"default",ctor);
    for(const auto& [key,value]:std::initializer_list<std::pair<const char*,const char*>>{{"VERTEX","vertex"},{"INSTANCE","instance"},{"SUBGROUP","subgroup"},{"INVOCATION_LOCAL","invocationLocal"},{"INVOCATION_SUBGROUP","invocationSubgroup"},{"DRAW","draw"}})own(ctor,key,value);
    const auto immutable=nativeTSLCoreBinding("nodeImmutable");for(const auto& [key,constant]:std::initializer_list<std::pair<const char*,const char*>>{{"vertexIndex","VERTEX"},{"instanceIndex","INSTANCE"},{"subgroupIndex","SUBGROUP"},{"invocationSubgroupIndex","INVOCATION_SUBGROUP"},{"invocationLocalIndex","INVOCATION_LOCAL"},{"drawIndex","DRAW"}})own(bindings,key,immutable.call({}, {ctor,ctor.get(constant)}));
    auto module=JSValue::object();std::vector<std::string> names{"default","vertexIndex","instanceIndex","subgroupIndex","invocationSubgroupIndex","invocationLocalIndex","drawIndex"};std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();indexBindings()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeIndexNodeBinding(const std::string& name){const auto keeper=nativeIndexNodeModule();return indexBindings().lock()->get(name);}
JSValue nativeIndexNodeConstructor(){return nativeIndexNodeBinding("default");}
IndexNode::IndexNode(const JSValue& scope):object_(constructor().construct({scope})){}
JSValue IndexNode::generate(const JSValue& builder)const{return object_.get("generate").call(object_,{builder});}
