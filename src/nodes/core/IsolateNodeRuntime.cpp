#include "nodes/core/IsolateNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"
#include "utils.h"

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& self,const Args& args,const Args& captures){
    const auto name=captures[0].string();const auto builder=arg(args,0);
    if(name=="isCacheable")return false;
    if(name=="setParent"){auto receiver=self;receiver.set("parent",arg(args,0));return self;}
    if(name=="getParent")return self.get("parent");
    JSValue rest;if(name=="build"){rest=JSValue::array(args.empty()?0:static_cast<unsigned>(args.size()-1));for(std::size_t i=1;i<args.size();++i)own(rest,std::to_string(i-1).c_str(),args[i]);}
    const auto previous=invoke(builder,"getCache")();const auto getCache=invoke(builder,"getCacheFromNode");const auto cache=getCache({self,self.get("parent")});invoke(builder,"setCache")({cache});
    JSValue result;
    if(name=="generateNodeType")result=invoke(self.get("node"),"getNodeType")({builder});
    else if(name=="build"){const auto build=invoke(self.get("node"),"build");Args child{builder};each(rest,[&](const JSValue& value){child.push_back(value);});result=build(child);}
    else throw std::logic_error("Unknown IsolateNode method");
    invoke(builder,"setCache")({previous});return result;
}
}
JSValue nativeIsolateNodeConstructor(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"IsolateNode");
    auto ctor=makeConstructor("IsolateNode",1,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto self=superConstruct(target,captures);self.set("node",arg(args,0));self.set("parent",arg(args,1,true));self.set("isIsolateNode",true);return self;
    },{{"isCacheable",0},{"generateNodeType",1},{"build",1},{"setParent",1},{"getParent",0}},method);weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeIsolateNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"IsolateNode exports");
    const auto keeper=nativeTSLCore(),ctor=nativeIsolateNodeConstructor(),nodeObject=nativeTSLCoreBinding("nodeObject");auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);
    const auto isolate=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{return captures[0].construct({captures[1].call({}, {arg(args,0)})});},{ctor,nodeObject},"isolate",1);own(bindings,"isolate",isolate);
    const auto cache=JSValue::constructor(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
        ThreeUtils::warn({"TSL: \"cache()\" has been deprecated. Use \"isolate()\" instead."});const auto node=captures[0].call({}, {arg(args,0)});return invoke(node,"setParent")({arg(args,1,true)});
    },{isolate},"cache",1,JSValue::ConstructorKind::Callable);own(bindings,"cache",cache);
    const auto add=nativeTSLCoreBinding("addMethodChaining");add.call({}, {"cache",cache});add.call({}, {"isolate",isolate});
    auto module=JSValue::object();own(module,"cache",cache);own(module,"default",ctor);own(module,"isolate",isolate);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeIsolateNodeBinding(const std::string& name){const auto keeper=nativeIsolateNodeModule();return lexical().lock()->get(name);}
