#include "nodes/accessors/BuiltinNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
}
JSValue nativeBuiltinNodeModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"BuiltinNode module");
    const auto base=nativeNodeConstructor(),tsl=nativeTSLBase();auto holder=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& c)->JSValue{
        auto self=superConstruct(target,c,{"float"});self.set("name",arg(args,0));self.set("isBuiltinNode",true);return self;
    },{holder},"BuiltinNode",1);
    holder.set("constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));
    for(const auto key:{"isCacheable","generate"}){JSPropertyDescriptor d;d.value=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{return c[0].strictEquals("isCacheable")?JSValue(false):self.get("name");},{key},key,0);d.writable=d.configurable=true;prototype.defineProperty(key,d);}
    const auto factory=nativeTSLBaseBinding("nodeProxy").call({}, {ctor});const auto builtin=invoke(factory,"setParameterLength")({1});
    auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);own(bindings,"builtin",builtin);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);
    auto module=JSValue::object();own(module,"builtin",builtin);own(module,"default",ctor);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeBuiltinNodeBinding(const std::string& name){const auto keeper=nativeBuiltinNodeModule();return lexical().lock()->get(name);}
JSValue nativeBuiltinNodeConstructor(){return nativeBuiltinNodeBinding("default");}
BuiltinNode::BuiltinNode(const JSValue& name):object_(constructor().construct({name})){}
const std::string& BuiltinNode::name()const{nameCache_=object_.get("name").toString();return nameCache_;}
bool BuiltinNode::isCacheable(){return false;}
std::string BuiltinNode::generate()const{return object_.get("generate").call(object_).toString();}
JSValue BuiltinNode::generate(const JSValue& builder)const{return object_.get("generate").call(object_,{builder});}
