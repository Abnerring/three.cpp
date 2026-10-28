#include "core/InstancedBufferAttribute.h"

InstancedBufferAttribute& InstancedBufferAttribute::copy(const BufferAttribute& source) {
    BufferAttribute::copy(source);
    const auto* instanced = dynamic_cast<const InstancedBufferAttribute*>(&source);
    meshPerAttribute = instanced ? instanced->meshPerAttribute : JSValue();
    return *this;
}
std::shared_ptr<BufferAttribute> InstancedBufferAttribute::clone() const {
    auto result = std::make_shared<InstancedBufferAttribute>(Float32Array(), itemSize, normalized);
    result->copy(*this);
    return result;
}
JsonValue InstancedBufferAttribute::toJSON() const {
    auto data = BufferAttribute::toJSON();
    if (const auto serialized = meshPerAttribute.toJSON()) data["meshPerAttribute"] = *serialized;
    data["isInstancedBufferAttribute"] = true;
    return data;
}

#include "core/NativeObject.h"
#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
#include "core/InstancedBufferAttributeSource.inc"
JSWeakValue& instancedAttributeLexical(){static thread_local JSWeakValue weak;return weak;}
}
JSValue nativeInstancedBufferAttributeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto m=weak.lock();m&&m->isObject())return *m;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"InstancedBufferAttribute module");
    const auto base=nativeBufferAttributeConstructor();auto holder=JSValue::object(nullptr);auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& a,const Args& c)->JSValue{const auto divisor=arg(a,3,1);auto self=superConstruct(target,c,{arg(a,0),arg(a,1),arg(a,2)});self.set("isInstancedBufferAttribute",true);self.set("meshPerAttribute",divisor);return self;},{holder},"InstancedBufferAttribute",3);holder.set("constructor",ctor);ctor.setPrototype(base);ctor.setFunctionSourceText(InstancedBufferAttributeSource::classSource);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));
    for(const auto* key:{"copy","toJSON"}){auto fn=JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue{auto self=receiver;const auto key=c[0];const auto super=c[1].getPrototype().get(key.toPropertyKey(),self);if(key.strictEquals("copy")){super.call(self,{arg(a,0)});self.set("meshPerAttribute",arg(a,0).get("meshPerAttribute"));return self;}auto data=super.call(self);data.set("meshPerAttribute",self.get("meshPerAttribute"));data.set("isInstancedBufferAttribute",true);return data;},{key,p},key,std::string(key)=="copy"?1:0);fn.setFunctionSourceText(std::string(key)=="copy"?InstancedBufferAttributeSource::copySource:InstancedBufferAttributeSource::toJSONSource);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;p.defineProperty(key,d);}
    auto bindings=JSValue::object(nullptr),module=JSValue::object();own(bindings,"InstancedBufferAttribute",ctor);own(module,"InstancedBufferAttribute",ctor);bindings.retainForHeapLifetime();instancedAttributeLexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeInstancedBufferAttributeBinding(const std::string& name){const auto keeper=nativeInstancedBufferAttributeModule();return instancedAttributeLexical().lock()->get(name);}
JSValue nativeInstancedBufferAttributeConstructor(){return nativeInstancedBufferAttributeBinding("InstancedBufferAttribute");}
