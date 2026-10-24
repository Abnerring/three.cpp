#include "nodes/core/UniformNode.h"

std::string UniformNode::uniformHash(const std::string& ordinaryHash) {
    // The ordinary hash, unless a derived class says otherwise.
    return ordinaryHash;
}

UniformNode::Shared UniformNode::sharedNode(bool hashAlreadyKnown, std::uint64_t registeredId,
                                            std::uint64_t ownId) {
    Shared out;
    if (hashAlreadyKnown) {
        // The first one registered is the one they all share.
        out.wasRegistered = false;
        out.nodeId = registeredId;
    } else {
        // Nothing is registered yet, so this node registers itself and becomes the shared one.
        out.wasRegistered = true;
        out.nodeId = ownId;
    }
    return out;
}

bool UniformNode::hashesMergeNodes() {
    return true;
}

std::string UniformNode::deprecationWarning() {
    return "TSL: \"label()\" has been deprecated. Use \"setName()\" instead.";
}

#include "nodes/core/InputNode.h"
#include "nodes/core/StackTrace.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/tsl/TSLCore.h"
#include "core/JSModuleObject.h"
#include "utils.h"
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
struct Invocation{JSValue object,function;JSValue operator()(const Args& args={})const{return function.call(object,args);}};
Invocation invoke(const JSValue& object,const char* key){return {object,object.get(key)};}
JSValue superCall(const JSValue& self,const char* key,const Args& args){return nativeUniformNodeConstructor().get("prototype").getPrototype().get(key,self).call(self,args);}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0].string();const auto builder=arg(args,0);
    if(name=="setName"){self.set("name",arg(args,0));return self;}
    if(name=="label"){ThreeUtils::warn({UniformNode::deprecationWarning(),nativeStackTraceConstructor().construct()});return invoke(self,"setName")({arg(args,0)});}
    if(name=="setGroup"){self.set("groupNode",arg(args,0));return self;}if(name=="getGroup")return self.get("groupNode");
    if(name=="getUniformHash")return invoke(self,"getHash")({builder});
    if(name=="getSharedNode"){
        const auto hash=invoke(self,"getUniformHash")({builder});auto shared=invoke(builder,"getNodeFromHash")({hash});
        if(shared.isUndefined()){invoke(builder,"setHashNode")({self,hash});shared=self;}return shared;
    }
    if(name=="onUpdate"){
        const auto bound=invoke(arg(args,0),"bind")({self});
        const auto callback=JSValue::function([](const JSValue&,const Args& values,const Args& captures)->JSValue{
            const auto value=captures[0].call({}, {arg(values,0),captures[1]});if(!value.isUndefined()){auto node=captures[1];node.set("value",value);}return {};
        },{bound,self},"",1);
        return superCall(self,"onUpdate",{callback,arg(args,1)});
    }
    if(name=="getInputType"){const auto type=superCall(self,"getInputType",{builder});return type.strictEquals("bool")?JSValue("uint"):type;}
    if(name=="generate"){
        const auto type=invoke(self,"getNodeType")({builder});const auto shared=invoke(self,"getSharedNode")({builder});const auto sharedType=invoke(shared,"getInputType")({builder});
        const auto getUniform=invoke(builder,"getUniformFromNode");const auto stage=builder.get("shaderStage");auto label=self.get("name");if(!label.toBoolean())label=builder.get("context").get("nodeName");
        const auto uniform=getUniform({shared,sharedType,stage,label});const auto uniformName=invoke(builder,"getPropertyName")({uniform});
        if(!builder.get("context").get("nodeName").isUndefined()){auto context=builder.get("context");if(!context.erase("nodeName"))throw JSTypeError("Cannot delete non-configurable nodeName");}
        auto snippet=uniformName;
        if(type.strictEquals("bool")){
            auto data=invoke(builder,"getDataFromNode")({self});auto property=data.get("propertyName");
            if(property.isUndefined()){
                const auto variable=invoke(builder,"getVarFromNode")({self,nullptr,"bool"});property=invoke(builder,"getPropertyName")({variable});data.set("propertyName",property);
                snippet=invoke(builder,"format")({uniformName,sharedType,type});const auto addLine=invoke(builder,"addLineFlowCode");const auto left=property.toString();const auto right=snippet.toString();addLine({left+" = "+right,self});
            }snippet=property;
        }
        return invoke(builder,"format")({snippet,type,arg(args,1)});
    }
    throw std::logic_error("Unknown UniformNode method");
}
}
JSValue nativeUniformNodeConstructor(){
    static thread_local JSWeakValue weak;static thread_local bool initializing=false;
    if(const auto ctor=weak.lock();ctor&&ctor->isConstructible())return *ctor;
    JSModuleInitializationScope scope(initializing,"UniformNode");auto state=JSValue::object(nullptr);const auto base=nativeInputNodeConstructor();const auto group=nativeObjectGroup();
    auto ctor=JSValue::builtinConstructor([](const JSValue& newTarget,const Args& args,const Args& captures)->JSValue{
        const auto type=arg(args,1);auto self=captures[0].get("constructor").getPrototype().construct({arg(args,0),type.isUndefined()?JSValue(nullptr):type},newTarget);self.set("isUniformNode",true);self.set("name","");self.set("groupNode",captures[1]);return self;
    },{state,group},"UniformNode",1);state.set("constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;type.getter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return "UniformNode";},{},"get type",0);type.configurable=true;ctor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"setName",1},{"label",1},{"setGroup",1},{"getGroup",0},{"getUniformHash",1},{"getSharedNode",1},{"onUpdate",2},{"getInputType",1},{"generate",2}}){JSPropertyDescriptor d;d.value=JSValue::function(method,{name},name,arity);d.writable=d.configurable=true;prototype.defineProperty(name,d);}
    ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeUniformFactory(){
    static thread_local JSWeakValue weak;if(const auto function=weak.lock();function&&function->isCallable())return *function;
    auto function=JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue{
        auto value=arg(args,0);const auto type=arg(args,1);
        const auto nodeType=nativeTSLCore().get("getConstNodeType").call({}, {type.toBoolean()?type:value});
        if(nodeType.strictEquals(value))value=nativeNodeUtils().get("getValueFromType").call({}, {nodeType});
        if(value.toBoolean()&&value.get("isNode").strictEquals(true)){
            auto state=JSValue::object(nullptr);state.set("value",value.get("value"));
            const auto traverse=value.get("traverse");
            const auto callback=JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{
                const auto node=arg(args,0);
                if(node.get("isConstNode").strictEquals(true)){auto state=c[0];state.set("value",node.get("value"));}
                return {};
            },{state},"",1);
            traverse.call(value,{callback});value=state.get("value");
        }
        return nativeUniformNodeConstructor().construct({value,nodeType});
    },{},"uniform",2);
    function.retainForHeapLifetime();weak=JSWeakValue(function);return function;
}
UniformNode::UniformNode(const JSValue& value,const JSValue& type):object_(constructor().construct({value,type})){}
