#include "nodes/display/ToneMappingNode.h"
#include "nodes/accessors/RendererReferenceNode.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"
#include "constants.h"
#include "utils.h"

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSWeakValue& importsWeak(){static thread_local JSWeakValue weak;return weak;}
JSValue imports(){
    if(const auto value=importsWeak().lock();value&&value->isObject())return *value;
    const auto node=nativeNodeConstructor(),tsl=nativeTSLCore(),renderer=nativeRendererReferenceNodeModule(),utils=nativeNodeUtils();auto table=JSValue::object(nullptr);
    own(table,"nodeObject",nativeTSLCoreBinding("nodeObject"));own(table,"vec4",nativeTSLCoreBinding("vec4"));own(table,"hash",nativeNodeUtilsBinding("hash"));
    own(table,"toneMappingExposure",nativeRendererReferenceNodeBinding("rendererReference").call({}, {"toneMappingExposure","float"}));
    table.retainForHeapLifetime();importsWeak()=JSWeakValue(table);return table;
}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0],table=captures[2];
    if(name.strictEquals("customCacheKey"))return table.get("hash").call({}, {self.get("_toneMapping")});
    if(name.strictEquals("setToneMapping")){self.set("_toneMapping",arg(args,0));return self;}
    if(name.strictEquals("getToneMapping"))return self.get("_toneMapping");
    if(name.strictEquals("setup")){
        const auto builder=arg(args,0);auto color=self.get("colorNode");if(!color.toBoolean())color=builder.get("context").get("color");const auto mapping=self.get("_toneMapping");
        if(mapping.strictEquals(NoToneMapping))return color;
        auto output=JSValue(nullptr);const auto fn=invoke(builder.get("renderer").get("library"),"getToneMappingFunction")({mapping});
        if(!fn.isNull()){
            const auto rgb=color.get("rgb"),exposure=self.get("exposureNode");const auto mapped=fn.call({}, {rgb,exposure});const auto alpha=color.get("a");output=table.get("vec4").call({}, {mapped,alpha});
        }else {ThreeUtils::error({"ToneMappingNode: Unsupported Tone Mapping configuration.",mapping});output=color;}return output;
    }
    throw std::logic_error("Unknown ToneMappingNode method");
}
}
JSValue nativeToneMappingNodeConstructor(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto table=imports();auto ctor=makeConstructor("ToneMappingNode",1,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        const auto exposure=args.size()>1&&!args[1].isUndefined()?args[1]:imports().get("toneMappingExposure");
        auto self=superConstruct(target,captures,{"vec3"});self.set("_toneMapping",arg(args,0));self.set("exposureNode",exposure);self.set("colorNode",arg(args,2,nullptr));return self;
    },{{"customCacheKey",0},{"setToneMapping",1},{"getToneMapping",0},{"setup",1}},method,{table});weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeToneMappingNodeModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ToneMappingNode exports");
    const auto ctor=nativeToneMappingNodeConstructor(),table=imports();auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);own(bindings,"toneMappingExposure",table.get("toneMappingExposure"));
    own(bindings,"toneMapping",JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
        const auto nodeObject=captures[1];const auto exposure=nodeObject.call({}, {arg(args,1)}),color=nodeObject.call({}, {arg(args,2)});return captures[0].construct({arg(args,0),exposure,color});
    },{ctor,table.get("nodeObject")},"toneMapping",3));
    const auto chain=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{return captures[0].call({}, {arg(args,1),arg(args,2),arg(args,0)});},{bindings.get("toneMapping")},"",3);
    nativeTSLCoreBinding("addMethodChaining").call({}, {"toneMapping",chain});
    auto module=JSValue::object();for(const auto* name:{"default","toneMapping","toneMappingExposure"})own(module,name,bindings.get(name));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeToneMappingNodeBinding(const std::string& name){const auto keeper=nativeToneMappingNodeModule();return lexical().lock()->get(name);}
