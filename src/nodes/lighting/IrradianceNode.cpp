#include "nodes/lighting/IrradianceNode.h"
#include "nodes/lighting/LightingNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
namespace { JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;} }
JSValue nativeIrradianceNodeModule(){
    using namespace LightingNodeRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"IrradianceNode module");
    const auto base=nativeLightingNodeBinding("default");auto bindings=JSValue::object(nullptr);
    const auto ctor=makeClass("IrradianceNode",1,base,+[](const JSValue& target,const Args& a,const Args& c)->JSValue{
        auto self=superConstruct(target,c);self.set("node",arg(a,0));return self;
    },{},IrradianceNodeSource::classSource,IrradianceNodeSource::typeSource);
    method(ctor.get("prototype"),"setup",1,+[](const JSValue& self,const Args& a,const Args&)->JSValue{
        const auto add=invoke(arg(a,0).get("context").get("irradiance"),"addAssign");add({self.get("node")});return {};
    },{},IrradianceNodeSource::setupSource);
    own(bindings,"default",ctor);return publish(bindings,lexical(),weak);
}
JSValue nativeIrradianceNodeBinding(const std::string& name){const auto keeper=nativeIrradianceNodeModule();return lexical().lock()->get(name);}
JSValue nativeIrradianceNodeConstructor(){return nativeIrradianceNodeBinding("default");}
IrradianceNode::IrradianceNode(const JSValue& value):object_(constructor().construct({value})){}
