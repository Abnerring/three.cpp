// Ported from the pinned three.js InspectorBase.js; see THREEJS-LICENSE.
#include "renderers/common/InspectorBase.h"
#include "core/EventDispatcher.h"
#include "nodes/utils/UtilityNodeRuntime.h"

namespace {
using namespace UtilityNodeRuntime;
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0];
    if(name.strictEquals("nodeFrame"))return self.get("_renderer").get("_nodes").get("nodeFrame");
    if(name.strictEquals("setRenderer")){self.set("_renderer",arg(args,0));return self;}
    if(name.strictEquals("getRenderer"))return self.get("_renderer");
    if(name.strictEquals("begin")){self.set("isRunning",true);return {};}
    if(name.strictEquals("finish")){self.set("isRunning",false);return {};}
    if(name.strictEquals("dispose")){
        if(self.get("isRunning").strictEquals(true))invoke(self,"finish")();
        const auto dispatch=invoke(self,"dispatchEvent");auto event=JSValue::object();own(event,"type","dispose");dispatch({event});
    }
    return {};
}
}
JSValue nativeInspectorBaseConstructor(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto base=EventDispatcher::runtimeConstructor();auto holder=JSValue::object(nullptr);
    auto constructor=JSValue::builtinConstructor(+[](const JSValue& target,const Args&,const Args& captures)->JSValue{
        auto self=captures[0].get("constructor").getPrototype().construct({},target);
        self.set("_renderer",nullptr);self.set("currentFrame",nullptr);self.set("isRunning",false);self.set("enabled",true);return self;
    },{holder},"InspectorBase",0);
    holder.set("constructor",constructor);constructor.setPrototype(base);auto prototype=constructor.get("prototype");prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor frame;frame.getter=JSValue::function(method,{"nodeFrame"},"get nodeFrame",0);frame.configurable=true;prototype.defineProperty("nodeFrame",frame);
    for(const auto* name:{"setRenderer","getRenderer","begin","finish","inspect","computeAsync","beginCompute","finishCompute","beginRender","finishRender","copyTextureToTexture","copyFramebufferToTexture","dispose"}){
        JSPropertyDescriptor d;d.value=JSValue::function(method,{name},name,std::string_view(name)=="setRenderer"?1:0);d.writable=d.configurable=true;prototype.defineProperty(name,d);
    }
    constructor.retainForHeapLifetime();weak=JSWeakValue(constructor);return constructor;
}
JSValue nativeInspectorBaseModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    auto module=JSValue::object();own(module,"default",nativeInspectorBaseConstructor());module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeInspectorBaseBinding(const std::string& name){
    if(name=="default")return nativeInspectorBaseConstructor();return {};
}
