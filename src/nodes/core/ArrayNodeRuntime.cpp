#include "nodes/core/ArrayNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& self,const Args& args,const Args& captures){
    const auto name=captures[0].string();const auto builder=arg(args,0);
    if(name=="getArrayCount")return self.get("count");
    if(name=="generateNodeType"){
        if(self.get("nodeType").isNull())return invoke(self.get("values").get("0"),"getNodeType")({builder});
        return self.get("nodeType");
    }
    if(name=="getElementType")return invoke(self,"getNodeType")({builder});
    if(name=="getMemberType"){
        if(self.get("nodeType").isNull())return invoke(self.get("values").get("0"),"getMemberType")({builder,arg(args,1)});
        return captures[1].getPrototype().get("getMemberType",self).call(self,{builder,arg(args,1)});
    }
    if(name=="generate"){
        const auto type=invoke(self,"getNodeType")({builder});const auto generate=invoke(builder,"generateArray");const auto count=self.get("count"),values=self.get("values");
        return generate({type,count,values});
    }
    throw std::logic_error("Unknown ArrayNode method");
}
}
JSValue nativeArrayNodeConstructor(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ArrayNode");
    auto ctor=makeConstructor("ArrayNode",2,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto self=superConstruct(target,captures,{arg(args,0)});self.set("count",arg(args,1));self.set("values",arg(args,2,nullptr));self.set("isArrayNode",true);return self;
    },{{"getArrayCount",0},{"generateNodeType",1},{"getElementType",1},{"getMemberType",2},{"generate",1}},method);
    weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeArrayNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ArrayNode exports");
    const auto keeper=nativeTSLCore(),ctor=nativeArrayNodeConstructor(),nodeObject=nativeTSLCoreBinding("nodeObject");auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);
    const auto factory=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
        if(args.size()==1){
            const auto map=invoke(args[0],"map");const auto callback=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{return captures[0].call({}, {arg(args,0)});},{captures[1]},"",1);
            const auto values=map({callback});return captures[0].construct({nullptr,values.get("length"),values});
        }
        return captures[0].construct({arg(args,0),arg(args,1)});
    },{ctor,nodeObject},"array",0);own(bindings,"array",factory);
    const auto toArray=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
        const auto array=JSValue::intrinsicArrayConstructor().call({}, {arg(args,1)});const auto filled=invoke(array,"fill")({arg(args,0)});return captures[0].call({}, {filled});
    },{factory},"",2);nativeTSLCoreBinding("addMethodChaining").call({}, {"toArray",toArray});
    auto module=JSValue::object();own(module,"array",factory);own(module,"default",ctor);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeArrayNodeBinding(const std::string& name){const auto keeper=nativeArrayNodeModule();return lexical().lock()->get(name);}
