// Ported from the pinned three.js RenderOutputNode.js; see THREEJS-LICENSE.
#include "nodes/display/RenderOutputNode.h"
#include "nodes/display/PremultiplyAlphaFunctions.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"
#include "math/ColorManagement.h"
#include "constants.h"

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0];
    if(name.strictEquals("setToneMapping")){self.set("_toneMapping",arg(args,0));return self;}
    if(name.strictEquals("getToneMapping"))return self.get("_toneMapping");
    if(name.strictEquals("setup")){
        const auto context=arg(args,0).get("context"),bindings=captures[2];
        auto output=self.get("colorNode");if(!output.toBoolean())output=context.get("color");
        const auto rgb=output.get("rgb");const auto clamp=invoke(output.get("a"),"clamp");const auto alpha=clamp({0,1});output=bindings.get("vec4").call({}, {rgb,alpha});
        if(self.get("premultipliedAlpha").strictEquals(true))output=bindings.get("unpremultiplyAlpha").call({}, {output});
        auto mapping=!self.get("_toneMapping").isNull()?self.get("_toneMapping"):context.get("toneMapping");if(!mapping.toBoolean())mapping=NoToneMapping;
        auto space=!self.get("outputColorSpace").isNull()?self.get("outputColorSpace"):context.get("outputColorSpace");if(!space.toBoolean())space=NoColorSpace;
        if(!mapping.strictEquals(NoToneMapping))output=invoke(output,"toneMapping")({mapping});
        if(!space.strictEquals(NoColorSpace)&&!space.strictEquals(bindings.get("management").get("workingColorSpace")))output=invoke(output,"workingToColorSpace")({space});
        if(self.get("premultipliedAlpha").strictEquals(true))output=bindings.get("premultiplyAlpha").call({}, {output});
        return output;
    }
    throw std::logic_error("Unknown RenderOutputNode method");
}
}
JSValue nativeRenderOutputNodeConstructor(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto tsl=nativeTSLCore(),alpha=nativePremultiplyAlphaFunctions();auto bindings=JSValue::object(nullptr);
    own(bindings,"vec4",nativeTSLCoreBinding("vec4"));own(bindings,"management",nativeColorManagement());
    for(const auto* name:{"premultiplyAlpha","unpremultiplyAlpha"})own(bindings,name,nativePremultiplyAlphaFunctionsBinding(name));
    auto constructor=makeConstructor("RenderOutputNode",3,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto self=superConstruct(target,captures,{"vec4"});self.set("colorNode",arg(args,0));self.set("_toneMapping",arg(args,1));self.set("outputColorSpace",arg(args,2));
        self.set("premultipliedAlpha",true);self.set("isRenderOutputNode",true);return self;
    },{{"setToneMapping",1},{"getToneMapping",0},{"setup",1}},method,{bindings});weak=JSWeakValue(constructor);return constructor;
}
JSValue nativeRenderOutputNodeModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"RenderOutputNode exports");
    const auto constructor=nativeRenderOutputNodeConstructor(),nodeObject=nativeTSLCoreBinding("nodeObject");auto bindings=JSValue::object(nullptr);own(bindings,"default",constructor);
    own(bindings,"renderOutput",JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
        const auto color=captures[1].call({}, {arg(args,0)});return captures[0].construct({color,arg(args,1,nullptr),arg(args,2,nullptr)});
    },{constructor,nodeObject},"renderOutput",1));nativeTSLCoreBinding("addMethodChaining").call({}, {"renderOutput",bindings.get("renderOutput")});
    auto module=JSValue::object();for(const auto* name:{"default","renderOutput"})own(module,name,bindings.get(name));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeRenderOutputNodeBinding(const std::string& name){const auto keeper=nativeRenderOutputNodeModule();return lexical().lock()->get(name);}
RenderOutputNode::RenderOutputNode(const JSValue& color,const JSValue& mapping,const JSValue& space):object_(constructor().construct({color,mapping,space})){}
