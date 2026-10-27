#include "nodes/lighting/LightingNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
namespace { JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;} }
JSValue nativeLightingNodeModule(){
    using namespace LightingNodeRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"LightingNode module");
    const auto base=nativeNodeConstructor();auto bindings=JSValue::object(nullptr);
    const auto ctor=makeClass("LightingNode",0,base,+[](const JSValue& target,const Args&,const Args& c)->JSValue{
        auto self=superConstruct(target,c,{"vec3"});self.set("isLightingNode",true);return self;
    },{},LightingNodeSource::classSource,LightingNodeSource::typeSource);
    method(ctor.get("prototype"),"isCacheable",0,+[](const JSValue&,const Args&,const Args&)->JSValue{return false;},{},LightingNodeSource::isCacheableSource);
    own(bindings,"default",ctor);return publish(bindings,lexical(),weak);
}
JSValue nativeLightingNodeBinding(const std::string& name){const auto keeper=nativeLightingNodeModule();return lexical().lock()->get(name);}
JSValue nativeLightingNodeConstructor(){return nativeLightingNodeBinding("default");}
LightingNode::LightingNode():object_(constructor().construct()){}
