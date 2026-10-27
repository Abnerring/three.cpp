#include "nodes/utils/EventNode.h"

EventNode::Slot EventNode::slotFor(const std::string& eventType) {
    // The plain slot for the three unary names, the after slot for the one after name, the
    // before slot for the three before ones, and no slot for the two pipeline names, which are
    // handled by pushing a callback rather than by filling a slot.
    if (eventType == kObject || eventType == kMaterial || eventType == kFrame) return Slot::Plain;
    if (eventType == kAfterObject) return Slot::After;
    if (eventType == kBeforeObject || eventType == kBeforeMaterial || eventType == kBeforeFrame) {
        return Slot::Before;
    }
    if (eventType == kBeforePipeline || eventType == kAfterPipeline) return Slot::Pipeline;
    return Slot::None;
}

EventNode::Kind EventNode::kindFor(const std::string& eventType) {
    // The material names use the render kind rather than a material one, since there is no
    // material kind to use.
    if (eventType == kObject || eventType == kAfterObject || eventType == kBeforeObject) {
        return Kind::Object;
    }
    if (eventType == kMaterial || eventType == kBeforeMaterial) return Kind::Render;
    if (eventType == kFrame || eventType == kBeforeFrame) return Kind::Frame;
    return Kind::None;
}

bool EventNode::isKnown(const std::string& eventType) {
    return slotFor(eventType) != Slot::None;
}

bool EventNode::isCacheable() {
    return false;
}
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
namespace { JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;} }
JSValue nativeEventNodeModule(){
    using namespace LightingNodeRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"EventNode module");
    const auto base=nativeNodeConstructor(),types=nativeNodeConstantsBinding("NodeUpdateType");auto bindings=JSValue::object(nullptr);
    auto ctor=makeClass("EventNode",2,base,+[](const JSValue& target,const Args& a,const Args& c)->JSValue{
        const auto event=arg(a,0),callback=arg(a,1);auto self=superConstruct(target,c,{"void"});self.set("eventType",event);self.set("callback",callback);
        const auto ctor=c[0].get("constructor");
        for(const auto& [name,slot,type]:std::initializer_list<std::tuple<const char*,const char*,const char*>>{
            {"OBJECT","updateType","OBJECT"},{"MATERIAL","updateType","RENDER"},{"FRAME","updateType","FRAME"},{"AFTER_OBJECT","updateAfterType","OBJECT"},
            {"BEFORE_OBJECT","updateBeforeType","OBJECT"},{"BEFORE_MATERIAL","updateBeforeType","RENDER"},{"BEFORE_FRAME","updateBeforeType","FRAME"}}){
            if(event.strictEquals(ctor.get(name))){self.set(slot,c[1].get(type));break;}
        }return self;
    },{types},EventNodeSource::classSource,EventNodeSource::typeSource);
    const auto prototype=ctor.get("prototype");
    method(prototype,"isCacheable",0,+[](const JSValue&,const Args&,const Args&)->JSValue{return false;},{},EventNodeSource::isCacheableSource);
    method(prototype,"setup",1,+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
        const auto event=self.get("eventType"),callback=self.get("callback");const auto ctor=c[0];
        if(event.strictEquals(ctor.get("BEFORE_RENDER_PIPELINE"))){const auto callbacks=arg(a,0).get("context").get("onBeforePipelineCallbacks");if(callbacks.toBoolean())invoke(callbacks,"push")({callback});}
        else if(event.strictEquals(ctor.get("AFTER_RENDER_PIPELINE"))){const auto callbacks=arg(a,0).get("context").get("onAfterPipelineCallbacks");if(callbacks.toBoolean())invoke(callbacks,"push")({callback});}
        return c[1].getPrototype().get("setup",self).call(self,{arg(a,0)});
    },{ctor,prototype},EventNodeSource::setupSource);
    for(const auto& [name,source]:std::initializer_list<std::pair<const char*,const char*>>{{"update",EventNodeSource::updateSource},{"updateBefore",EventNodeSource::updateBeforeSource},{"updateAfter",EventNodeSource::updateAfterSource}})
        method(prototype,name,1,+[](const JSValue& self,const Args& a,const Args&)->JSValue{invoke(self,"callback")({arg(a,0)});return {};},{},source);
    for(const auto& [name,value]:std::initializer_list<std::pair<const char*,const char*>>{{"OBJECT","object"},{"MATERIAL","material"},{"FRAME","frame"},{"AFTER_OBJECT","afterObject"},{"BEFORE_OBJECT","beforeObject"},{"BEFORE_MATERIAL","beforeMaterial"},{"BEFORE_FRAME","beforeFrame"},{"BEFORE_RENDER_PIPELINE","beforeRenderPipeline"},{"AFTER_RENDER_PIPELINE","afterRenderPipeline"}})own(ctor,name,value);
    auto create=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{return invoke(c[0].construct({arg(a,0),arg(a,1)}),"toStack")();},{ctor},"createEvent",2);
    create.setFunctionSourceText(EventNodeSource::createEventSource);
    // Namespace enumeration is lexicographic; factories are declared in source order.
    auto functions=JSValue::object(nullptr);
    for(const auto& [name,type,source]:std::initializer_list<std::tuple<const char*,const char*,const char*>>{
        {"OnObjectUpdate","OBJECT",EventNodeSource::OnObjectUpdateSource},{"OnMaterialUpdate","MATERIAL",EventNodeSource::OnMaterialUpdateSource},
        {"OnFrameUpdate","FRAME",EventNodeSource::OnFrameUpdateSource},{"OnAfterObjectUpdate","AFTER_OBJECT",EventNodeSource::OnAfterObjectUpdateSource},
        {"OnBeforeObjectUpdate","BEFORE_OBJECT",EventNodeSource::OnBeforeObjectUpdateSource},{"OnBeforeMaterialUpdate","BEFORE_MATERIAL",EventNodeSource::OnBeforeMaterialUpdateSource},
        {"OnBeforeFrameUpdate","BEFORE_FRAME",EventNodeSource::OnBeforeFrameUpdateSource},{"OnBeforeRenderPipeline","BEFORE_RENDER_PIPELINE",EventNodeSource::OnBeforeRenderPipelineSource},
        {"OnAfterRenderPipeline","AFTER_RENDER_PIPELINE",EventNodeSource::OnAfterRenderPipelineSource}}){
        auto fn=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{const auto type=c[0].get(c[2]);return c[1].call({}, {type,arg(a,0)});},{ctor,create,type},name,1);
        fn.setFunctionSourceText(source);own(functions,name,fn);
    }
    for(const auto* name:{"OnAfterObjectUpdate","OnAfterRenderPipeline","OnBeforeFrameUpdate","OnBeforeMaterialUpdate","OnBeforeObjectUpdate","OnBeforeRenderPipeline","OnFrameUpdate","OnMaterialUpdate","OnObjectUpdate"})own(bindings,name,functions.get(name));
    own(bindings,"default",ctor);return publish(bindings,lexical(),weak);
}
JSValue nativeEventNodeBinding(const std::string& name){const auto keeper=nativeEventNodeModule();return lexical().lock()->get(name);}
JSValue nativeEventNodeConstructor(){return nativeEventNodeBinding("default");}
EventNode::EventNode(const JSValue& type,const JSValue& callback):object_(constructor().construct({type,callback})){}
