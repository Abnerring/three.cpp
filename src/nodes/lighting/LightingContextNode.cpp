#include "nodes/lighting/LightingContextNode.h"
#include "nodes/core/ContextNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
namespace { JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;} }
JSValue nativeLightingContextNodeModule(){
    using namespace LightingNodeRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"LightingContextNode module");
    const auto context=nativeContextNodeModule(),tsl=nativeTSLBase();const auto base=nativeContextNodeBinding("default");
    auto bindings=JSValue::object(nullptr);const auto vec3=nativeTSLBaseBinding("vec3"),number=nativeTSLBaseBinding("float");
    const auto ctor=makeClass("LightingContextNode",1,base,+[](const JSValue& target,const Args& a,const Args& c)->JSValue{
        const auto lights=arg(a,0),model=arg(a,1,nullptr),material=arg(a,2,JSValue::array()),backdrop=arg(a,3,nullptr),alpha=arg(a,4,nullptr);
        auto self=superConstruct(target,c,{lights});self.set("lightingModel",model);self.set("materialLightings",material);self.set("backdropNode",backdrop);self.set("backdropAlphaNode",alpha);self.set("_value",nullptr);return self;
    },{},LightingContextNodeSource::classSource,LightingContextNodeSource::typeSource);
    const auto prototype=ctor.get("prototype");
    method(prototype,"getContext",0,+[](const JSValue& self,const Args&,const Args& c)->JSValue{
        const auto material=self.get("materialLightings"),backdrop=self.get("backdropNode"),alpha=self.get("backdropAlphaNode");
        auto reflected=JSValue::object();for(const auto* name:{"directDiffuse","directSpecular","indirectDiffuse","indirectSpecular"})own(reflected,name,invoke(c[0].call(),"toVar")({name}));
        auto value=JSValue::object();for(const auto* name:{"radiance","irradiance","iblIrradiance"})own(value,name,invoke(c[0].call(),"toVar")({name}));
        own(value,"ambientOcclusion",invoke(c[1].call({}, {1}),"toVar")({"ambientOcclusion"}));own(value,"reflectedLight",reflected);own(value,"materialLightings",material);own(value,"backdrop",backdrop);own(value,"backdropAlpha",alpha);return value;
    },{vec3,number},LightingContextNodeSource::getContextSource);
    method(prototype,"setup",1,+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
        auto receiver=self;auto value=self.get("_value");if(!value.toBoolean()){value=invoke(self,"getContext")();receiver.set("_value",value);}receiver.set("value",value);
        auto target=self.get("value");const auto model=self.get("lightingModel");target.set("lightingModel",model.toBoolean()?model:arg(a,0).get("context").get("lightingModel"));
        return c[0].getPrototype().get("setup",self).call(self,{arg(a,0)});
    },{prototype},LightingContextNodeSource::setupSource);
    own(bindings,"default",ctor);own(bindings,"lightingContext",nativeTSLBaseBinding("nodeProxy").call({}, {ctor}));return publish(bindings,lexical(),weak);
}
JSValue nativeLightingContextNodeBinding(const std::string& name){const auto keeper=nativeLightingContextNodeModule();return lexical().lock()->get(name);}
JSValue nativeLightingContextNodeConstructor(){return nativeLightingContextNodeBinding("default");}
LightingContextNode::LightingContextNode(const JSValue& lights,const JSValue& model,const JSValue& material,const JSValue& backdrop,const JSValue& alpha):object_(constructor().construct({lights,model,material,backdrop,alpha})){}
