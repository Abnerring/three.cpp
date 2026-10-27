#include "nodes/code/ExpressionNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& self,const Args& args,const Args& captures){
    if(captures[0].strictEquals("isCacheable"))return false;
    const auto builder=arg(args,0),type=invoke(self,"getNodeType")({builder}),snippet=self.get("snippet");
    if(type.strictEquals("void")){invoke(builder,"addLineFlowCode")({snippet,self});return {};}
    return invoke(builder,"format")({snippet,type,arg(args,1)});
}
}
JSValue nativeExpressionNodeConstructor(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    auto constructor=makeConstructor("ExpressionNode",0,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto self=superConstruct(target,captures,{arg(args,1,"void")});self.set("snippet",arg(args,0,""));return self;
    },{{"isCacheable",0},{"generate",2}},method);weak=JSWeakValue(constructor);return constructor;
}
JSValue nativeExpressionNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ExpressionNode exports");
    const auto tsl=nativeTSLCore(),constructor=nativeExpressionNodeConstructor();auto bindings=JSValue::object();own(bindings,"default",constructor);
    const auto factory=nativeTSLCoreBinding("nodeProxy").call({}, {constructor});own(bindings,"expression",invoke(factory,"setParameterLength")({1,2}));
    auto module=JSValue::object();for(const auto& key:bindings.ownPropertyNames())own(module,key.c_str(),bindings.get(key));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeExpressionNodeBinding(const std::string& name){const auto keeper=nativeExpressionNodeModule();return lexical().lock()->get(name);}
ExpressionNode::ExpressionNode(const JSValue& snippet,const JSValue& type):object_(constructor().construct({snippet,type})){}
JSValue ExpressionNode::generate(const JSValue& builder,const JSValue& output)const{return object_.get("generate").call(object_,{builder,output});}
