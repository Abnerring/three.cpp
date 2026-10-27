#include "nodes/accessors/ReferenceBaseNode.h"
#include "nodes/accessors/ReferenceElementNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0],imports=captures[2];
    if(name.strictEquals("isCacheable"))return false;
    if(name.strictEquals("setGroup")){self.set("group",arg(args,0));return self;}
    if(name.strictEquals("element")){
        const auto index=imports.get("nodeObject").call({}, {arg(args,0)});return imports.get("ReferenceElementNode").construct({self,index});
    }
    if(name.strictEquals("setNodeType")){
        const auto node=imports.get("uniform").call({}, {nullptr,arg(args,0)});
        if(!self.get("group").isNull())invoke(node,"setGroup")({self.get("group")});
        self.set("node",node);return {};
    }
    if(name.strictEquals("generateNodeType")){
        const auto builder=arg(args,0);
        if(self.get("node").isNull()){invoke(self,"updateReference")({builder});invoke(self,"updateValue")();}
        return invoke(self.get("node"),"getNodeType")({builder});
    }
    if(name.strictEquals("getValueFromReference")){
        const auto object=args.empty()||args[0].isUndefined()?self.get("reference"):args[0];const auto properties=self.get("properties");auto value=object.get(properties.get("0"));
        for(double i=1;less(i,properties.get("length"));++i)value=value.get(properties.get(JSValue(i).toString()));return value;
    }
    if(name.strictEquals("updateReference")){
        self.set("reference",!self.get("object").isNull()?self.get("object"):arg(args,0).get("object"));return self.get("reference");
    }
    if(name.strictEquals("setup")){invoke(self,"updateValue")();return self.get("node");}
    if(name.strictEquals("update")){invoke(self,"updateValue")();return {};}
    if(name.strictEquals("updateValue")){
        if(self.get("node").isNull()){const auto set=invoke(self,"setNodeType");set({self.get("uniformType")});}
        const auto value=invoke(self,"getValueFromReference")();
        if(invoke(JSValue::intrinsicArrayConstructor(),"isArray")({value}).toBoolean()){auto node=self.get("node");node.set("array",value);}
        else {auto node=self.get("node");node.set("value",value);}return {};
    }
    throw std::logic_error("Unknown ReferenceBaseNode method");
}
}
JSValue nativeReferenceBaseNodeConstructor(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto node=nativeNodeConstructor(),constants=nativeNodeConstants(),uniformConstructor=nativeUniformNodeConstructor(),uniform=nativeUniformFactory(),tsl=nativeTSLCore(),element=nativeReferenceElementNodeModule();
    auto imports=JSValue::object(nullptr);own(imports,"uniform",uniform);own(imports,"nodeObject",nativeTSLCoreBinding("nodeObject"));own(imports,"ReferenceElementNode",nativeReferenceElementNodeBinding("default"));
    auto ctor=makeConstructor("ReferenceBaseNode",2,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto self=superConstruct(target,captures);const auto property=arg(args,0),object=arg(args,2,nullptr);
        self.set("property",property);self.set("uniformType",arg(args,1));self.set("object",object);self.set("count",arg(args,3,nullptr));
        self.set("properties",invoke(property,"split")({"."}));self.set("reference",object);self.set("node",nullptr);self.set("group",nullptr);self.set("updateType",nativeNodeConstantsBinding("NodeUpdateType").get("OBJECT"));return self;
    },{{"isCacheable",0},{"setGroup",1},{"element",1},{"setNodeType",1},{"generateNodeType",1},{"getValueFromReference",0},{"updateReference",1},{"setup",0},{"update",0},{"updateValue",0}},method,{imports});
    weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeReferenceBaseNodeModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ReferenceBaseNode exports");
    const auto ctor=nativeReferenceBaseNodeConstructor();auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);
    for(const auto* name:{"reference","referenceBuffer"})own(bindings,name,JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
        if(captures[1].strictEquals("reference"))return captures[0].construct({arg(args,0),arg(args,1),arg(args,2)});
        return captures[0].construct({arg(args,0),arg(args,1),arg(args,3),arg(args,2)});
    },{ctor,name},name,std::string_view(name)=="reference"?3:4));
    auto module=JSValue::object();for(const auto* name:{"default","reference","referenceBuffer"})own(module,name,bindings.get(name));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeReferenceBaseNodeBinding(const std::string& name){const auto keeper=nativeReferenceBaseNodeModule();return lexical().lock()->get(name);}
