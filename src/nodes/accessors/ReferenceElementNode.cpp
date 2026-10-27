#include "nodes/accessors/ReferenceElementNode.h"
#include "nodes/utils/ArrayElementNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& self,const Args& args,const Args& captures){
    const auto name=captures[0],builder=arg(args,0);
    if(name.strictEquals("generateNodeType"))return self.get("referenceNode").get("uniformType");
    if(name.strictEquals("generate")){
        const auto snippet=captures[1].getPrototype().get("generate",self).call(self,{builder});
        const auto arrayType=invoke(self.get("referenceNode"),"getNodeType")({builder});
        const auto elementType=invoke(self,"getNodeType")({builder});
        return invoke(builder,"format")({snippet,arrayType,elementType});
    }
    throw std::logic_error("Unknown ReferenceElementNode method");
}
}
JSValue nativeReferenceElementNodeConstructor(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto base=nativeArrayElementNodeConstructor();
    auto ctor=makeConstructor("ReferenceElementNode",2,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto self=superConstruct(target,captures,{arg(args,0),arg(args,1)});self.set("referenceNode",arg(args,0));self.set("isReferenceElementNode",true);return self;
    },{{"generateNodeType",0},{"generate",1}},method);
    ctor.setPrototype(base);ctor.get("prototype").setPrototype(base.get("prototype"));weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeReferenceElementNodeModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    const auto ctor=nativeReferenceElementNodeConstructor();auto bindings=JSValue::object(nullptr),module=JSValue::object();own(bindings,"default",ctor);own(module,"default",ctor);
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeReferenceElementNodeBinding(const std::string& name){const auto keeper=nativeReferenceElementNodeModule();return lexical().lock()->get(name);}
bool ReferenceElementNode::needsFormatting(const std::string& arrayType,const std::string& elementType){return arrayType!=elementType;}
