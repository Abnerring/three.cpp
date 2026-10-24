#include "renderers/common/UniformBuffer.h"
#include "renderers/common/Buffer.h"

JSValue nativeUniformBufferConstructor(){
    static thread_local JSWeakValue weak;
    if(const auto ctor=weak.lock();ctor&&ctor->isConstructible())return *ctor;
    auto state=JSValue::object(nullptr);const auto base=nativeRendererBufferConstructor();
    auto ctor=JSValue::builtinConstructor([](const JSValue& target,const std::vector<JSValue>& args,const std::vector<JSValue>& c)->JSValue{
        const auto name=args.empty()?JSValue{}:args[0];
        const auto buffer=args.size()>1&&!args[1].isUndefined()?args[1]:JSValue(nullptr);
        auto self=c[0].get("constructor").getPrototype().construct({name,buffer},target);self.set("isUniformBuffer",true);return self;
    },{state},"UniformBuffer",1);
    state.set("constructor",ctor);ctor.setPrototype(base);ctor.get("prototype").setPrototype(base.get("prototype"));
    ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;
}
