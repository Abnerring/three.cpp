#include "nodes/accessors/RendererReferenceNode.h"
#include "nodes/accessors/ReferenceBaseNode.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& receiver,const Args& args,const Args&){
    auto self=receiver;self.set("reference",!self.get("renderer").isNull()?self.get("renderer"):arg(args,0).get("renderer"));return self.get("reference");
}
}
JSValue nativeRendererReferenceNodeConstructor(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto base=nativeReferenceBaseNodeBinding("default"),group=nativeRenderGroup();
    auto ctor=makeConstructor("RendererReferenceNode",2,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        const auto renderer=arg(args,2,nullptr);auto self=superConstruct(target,captures,{arg(args,0),arg(args,1),renderer});self.set("renderer",renderer);invoke(self,"setGroup")({nativeRenderGroup()});return self;
    },{{"updateReference",1}},method);ctor.setPrototype(base);ctor.get("prototype").setPrototype(base.get("prototype"));weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeRendererReferenceNodeModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"RendererReferenceNode exports");
    const auto ctor=nativeRendererReferenceNodeConstructor();auto bindings=JSValue::object(nullptr),module=JSValue::object();own(bindings,"default",ctor);
    own(bindings,"rendererReference",JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{return captures[0].construct({arg(args,0),arg(args,1),arg(args,2,nullptr)});},{ctor},"rendererReference",2));
    for(const auto* name:{"default","rendererReference"})own(module,name,bindings.get(name));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeRendererReferenceNodeBinding(const std::string& name){const auto keeper=nativeRendererReferenceNodeModule();return lexical().lock()->get(name);}
