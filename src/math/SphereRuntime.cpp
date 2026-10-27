// Native managed Sphere operations used by Object3DNode; see THREEJS-LICENSE.
#include "math/Sphere.h"
#include "core/JSModuleObject.h"

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,unsigned i){return i<a.size()?a[i]:JSValue{};}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto source=arg(args,0);
    if(captures[0].strictEquals("copy")){
        const auto center=self.get("center"),copy=center.get("copy"),sourceCenter=source.get("center");copy.call(center,{sourceCenter});self.set("radius",source.get("radius"));return self;
    }
    const auto center=self.get("center");center.get("applyMatrix4").call(center,{source});
    const auto radius=self.get("radius");const auto scale=source.get("getMaxScaleOnAxis").call(source);
    const auto left=radius.toNumber(),right=scale.toNumber();self.set("radius",left*right);return self;
}
}
JSValue nativeSphereConstructor(){
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Sphere managed constructor");
    const auto vector=nativeVector3Constructor();
    auto constructor=JSValue::constructor(+[](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
        auto self=receiver;const auto input=arg(args,0),center=input.isUndefined()?captures[0].construct():input;
        const auto radius=arg(args,1);self.set("isSphere",true);self.set("center",center);self.set("radius",radius.isUndefined()?JSValue(-1):radius);return {};
    },{vector},"Sphere",0);
    auto prototype=constructor.get("prototype");for(const auto* name:{"copy","applyMatrix4"}){JSPropertyDescriptor d;d.value=JSValue::function(method,{name},name,1);d.writable=d.configurable=true;prototype.defineProperty(name,d);}
    constructor.retainForHeapLifetime();weak=JSWeakValue(constructor);return constructor;
}
