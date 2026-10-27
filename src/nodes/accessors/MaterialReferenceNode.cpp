#include "nodes/accessors/MaterialReferenceNode.h"
#include "nodes/accessors/ReferenceNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
#include "MaterialReferenceNodeSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
}
JSValue nativeMaterialReferenceNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"MaterialReferenceNode module");const auto reference=nativeReferenceNodeModule(),base=nativeReferenceNodeBinding("default");auto holder=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        const auto material=arg(args,2,nullptr);auto self=captures[0].get("constructor").getPrototype().construct({arg(args,0),arg(args,1),material},target);self.set("material",material);self.set("isMaterialReferenceNode",true);return self;
    },{holder},"MaterialReferenceNode",2);holder.set("constructor",ctor);ctor.setPrototype(base);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(MaterialReferenceNodeSource::MaterialReferenceNode);
    JSPropertyDescriptor type;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "MaterialReferenceNode";},{},"get type",0);type.getter->setFunctionSourceText(MaterialReferenceNodeSource::get_type);type.configurable=true;ctor.defineProperty("type",type);
    JSPropertyDescriptor update;update.value=JSValue::function(+[](const JSValue& receiver,const Args& args,const Args&)->JSValue{auto self=receiver;self.set("reference",!self.get("material").isNull()?self.get("material"):arg(args,0).get("material"));return self.get("reference");},{},"updateReference",1);update.value->setFunctionSourceText(MaterialReferenceNodeSource::updateReference);update.writable=update.configurable=true;p.defineProperty("updateReference",update);
    own(bindings,"default",ctor);auto factory=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{return captures[0].construct({arg(args,0),arg(args,1),arg(args,2,nullptr)});},{ctor},"materialReference",2);factory.setFunctionSourceText(MaterialReferenceNodeSource::materialReference);own(bindings,"materialReference",factory);
    auto module=JSValue::object();for(const auto* key:{"default","materialReference"})own(module,key,bindings.get(key));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeMaterialReferenceNodeBinding(const std::string& name){const auto keeper=nativeMaterialReferenceNodeModule();return lexical().lock()->get(name);}
JSValue nativeMaterialReferenceNodeConstructor(){return nativeMaterialReferenceNodeBinding("default");}
