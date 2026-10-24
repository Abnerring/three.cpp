#include "renderers/common/Binding.h"
#include <cmath>
#include <cstdint>

namespace {
using Args=std::vector<JSValue>;
std::uint32_t uint32(const JSValue& value) {
    const auto number=value.toNumber(); if(!std::isfinite(number)||number==0)return 0;
    auto wrapped=std::fmod(std::trunc(number),4294967296.0);if(wrapped<0)wrapped+=4294967296.0;
    return static_cast<std::uint32_t>(wrapped);
}
JSValue bindingMethod(const JSValue& receiver,const Args& args,const Args& c) {
    auto self=receiver;
    if(c[0].strictEquals("setVisibility")) {
        const auto left=self.get("visibility"); const auto a=uint32(left);
        const auto b=uint32(args.empty()?JSValue{}:args[0]); const auto bits=a|b;
        self.set("visibility",bits>=2147483648u?double(bits)-4294967296.0:double(bits)); return {};
    }
    // The Object.assign getter precedes evaluation of the constructor argument.
    const auto object=JSValue::intrinsicObjectConstructor(); const auto assign=object.get("assign");
    const auto result=self.get("constructor").construct(); return assign.call(object,{result,self});
}
}
JSValue nativeBindingConstructor() {
    static thread_local JSWeakValue weak;
    if(const auto ctor=weak.lock();ctor&&ctor->isConstructible())return *ctor;
    auto ctor=JSValue::constructor([](const JSValue& receiver,const Args& args,const Args&)->JSValue{
        auto self=receiver;self.set("name",args.empty()||args[0].isUndefined()?JSValue(""):args[0]);self.set("visibility",0);return {};
    },{},"Binding",0);
    auto prototype=ctor.get("prototype");
    for(const auto* name:{"setVisibility","clone"}){JSPropertyDescriptor d;d.value=JSValue::function(bindingMethod,{name},name,std::string(name)=="clone"?0:1);d.writable=d.configurable=true;prototype.defineProperty(name,d);}
    ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;
}
