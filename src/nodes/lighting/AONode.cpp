#include "nodes/lighting/AONode.h"
#include "nodes/lighting/LightingNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
namespace { JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;} }
JSValue nativeAONodeModule(){
    using namespace LightingNodeRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"AONode module");
    const auto base=nativeLightingNodeBinding("default");auto bindings=JSValue::object(nullptr);
    const auto ctor=makeClass("AONode",0,base,+[](const JSValue& target,const Args& a,const Args& c)->JSValue{
        const auto ao=arg(a,0,nullptr);auto self=superConstruct(target,c);self.set("aoNode",ao);return self;
    },{},AONodeSource::classSource,AONodeSource::typeSource);
    method(ctor.get("prototype"),"setup",1,+[](const JSValue& self,const Args& a,const Args&)->JSValue{
        const auto multiply=invoke(arg(a,0).get("context").get("ambientOcclusion"),"mulAssign");multiply({self.get("aoNode")});return {};
    },{},AONodeSource::setupSource);
    own(bindings,"default",ctor);return publish(bindings,lexical(),weak);
}
JSValue nativeAONodeBinding(const std::string& name){const auto keeper=nativeAONodeModule();return lexical().lock()->get(name);}
JSValue nativeAONodeConstructor(){return nativeAONodeBinding("default");}
AONode::AONode(const JSValue& value):object_(constructor().construct({value})){}
