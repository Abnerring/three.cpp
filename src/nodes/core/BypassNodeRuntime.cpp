#include "nodes/core/BypassNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& self,const Args& args,const Args& captures){
    const auto name=captures[0].string();const auto builder=arg(args,0);
    if(name=="isCacheable")return false;
    if(name=="generateNodeType")return invoke(self.get("outputNode"),"getNodeType")({builder});
    if(name=="generate"){
        const auto snippet=invoke(self.get("callNode"),"build")({builder,"void"});
        if(!snippet.strictEquals(""))invoke(builder,"addLineFlowCode")({snippet,self});
        return invoke(self.get("outputNode"),"build")({builder});
    }
    throw std::logic_error("Unknown BypassNode method");
}
}
JSValue nativeBypassNodeConstructor(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"BypassNode");
    auto ctor=makeConstructor("BypassNode",2,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto self=superConstruct(target,captures);self.set("isBypassNode",true);self.set("outputNode",arg(args,0));self.set("callNode",arg(args,1));return self;
    },{{"isCacheable",0},{"generateNodeType",1},{"generate",1}},method);weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeBypassNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"BypassNode exports");
    const auto keeper=nativeTSLCore(),ctor=nativeBypassNodeConstructor();const auto proxy=nativeTSLCoreBinding("nodeProxy").call({}, {ctor});const auto bypass=invoke(proxy,"setParameterLength")({2});
    auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);own(bindings,"bypass",bypass);nativeTSLCoreBinding("addMethodChaining").call({}, {"bypass",bypass});
    auto module=JSValue::object();own(module,"bypass",bypass);own(module,"default",ctor);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeBypassNodeBinding(const std::string& name){const auto keeper=nativeBypassNodeModule();return lexical().lock()->get(name);}
