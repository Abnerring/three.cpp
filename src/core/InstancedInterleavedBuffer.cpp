// Direct source port; typed wrapper remains a view of the same managed object.
#include "core/InstancedInterleavedBuffer.h"
#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
#include "core/InstancedInterleavedBufferSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
}
JSValue nativeInstancedInterleavedBufferModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto m=weak.lock();m&&m->isObject())return *m;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"InstancedInterleavedBuffer module");
    const auto base=InterleavedBuffer::constructor();auto holder=JSValue::object(nullptr);auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& a,const Args& c)->JSValue{const auto divisor=arg(a,2,1);auto self=superConstruct(target,c,{arg(a,0),arg(a,1)});self.set("isInstancedInterleavedBuffer",true);self.set("meshPerAttribute",divisor);return self;},{holder},"InstancedInterleavedBuffer",2);holder.set("constructor",ctor);ctor.setPrototype(base);ctor.setFunctionSourceText(InstancedInterleavedBufferSource::classSource);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));
    for(const auto* key:{"copy","clone","toJSON"}){auto fn=JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue{auto self=receiver;const auto key=c[0];const auto super=c[1].getPrototype().get(key.toPropertyKey(),self);auto result=super.call(self,{arg(a,0)});if(key.strictEquals("copy")){self.set("meshPerAttribute",arg(a,0).get("meshPerAttribute"));return self;}if(key.strictEquals("clone")){result.set("meshPerAttribute",self.get("meshPerAttribute"));return result;}result.set("isInstancedInterleavedBuffer",true);result.set("meshPerAttribute",self.get("meshPerAttribute"));return result;},{key,p},key,1);fn.setFunctionSourceText(std::string(key)=="copy"?InstancedInterleavedBufferSource::copySource:std::string(key)=="clone"?InstancedInterleavedBufferSource::cloneSource:InstancedInterleavedBufferSource::toJSONSource);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;p.defineProperty(key,d);}
    auto bindings=JSValue::object(nullptr),module=JSValue::object();own(bindings,"InstancedInterleavedBuffer",ctor);own(module,"InstancedInterleavedBuffer",ctor);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeInstancedInterleavedBufferBinding(const std::string& name){const auto keeper=nativeInstancedInterleavedBufferModule();return lexical().lock()->get(name);}
JSValue nativeInstancedInterleavedBufferConstructor(){return nativeInstancedInterleavedBufferBinding("InstancedInterleavedBuffer");}
JSValue InstancedInterleavedBuffer::prototype(){return constructor().get("prototype");}
void InstancedInterleavedBuffer::initializeInstance(JSValue self,JSValue array,JSValue stride,JSValue meshPerAttribute){initialize(self,array,stride);self.set("isInstancedInterleavedBuffer",true);self.set("meshPerAttribute",meshPerAttribute.isUndefined()?JSValue(1):meshPerAttribute);}
InstancedInterleavedBuffer::InstancedInterleavedBuffer(JSValue array,JSValue stride,JSValue meshPerAttribute):InterleavedBuffer(constructor().construct({array,stride,meshPerAttribute}),Existing{}){}
