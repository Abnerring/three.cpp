#include "nodes/core/UniformGroupNode.h"

int UniformGroupNode::defaultOrder(bool shared) {
    // The shared factory's default is one lower than the plain one's.
    return shared ? kSharedOrder : kPlainOrder;
}

UniformGroupNode::Preset UniformGroupNode::frameGroup() {
    Preset preset;
    preset.name = "frame";
    preset.shared = true;
    preset.order = kSharedOrder;
    preset.updateType = "FRAME";
    return preset;
}

UniformGroupNode::Preset UniformGroupNode::renderGroup() {
    Preset preset;
    preset.name = "render";
    preset.shared = true;
    preset.order = kSharedOrder;
    preset.updateType = "RENDER";
    return preset;
}

UniformGroupNode::Preset UniformGroupNode::objectGroup() {
    Preset preset;
    preset.name = "object";
    // The odd one: not shared, and so at the plain order.
    preset.shared = false;
    preset.order = kPlainOrder;
    preset.updateType = "OBJECT";
    return preset;
}

bool UniformGroupNode::serialisesOrder() {
    // The order is not written out.
    return false;
}

bool UniformGroupNode::serialisesUpdateType() {
    // Nor is the update kind.
    return false;
}

bool UniformGroupNode::setsNeedsUpdateFlag() {
    return true;
}

bool UniformGroupNode::initialisesNeedsUpdate() {
    // Nothing in the constructor gives the flag a value, so before the first update it is absent.
    return false;
}

bool UniformGroupNode::isCacheable() {
    return false;
}

#include "nodes/core/Node.h"
#include "nodes/core/constants.h"
#include "core/JSModuleObject.h"
#include <algorithm>
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
JSValue defaulted(const Args& args,std::size_t i,const JSValue& fallback){const auto value=arg(args,i);return value.isUndefined()?fallback:value;}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0].string();
    if(name=="isCacheable")return false;if(name=="update"){self.set("needsUpdate",true);return {};}
    const auto data=arg(args,0);nativeUniformGroupNodeConstructor().get("prototype").getPrototype().get(name,self).call(self,{data});
    if(name=="serialize"){auto target=data;target.set("name",self.get("name"));target.set("version",self.get("version"));target.set("shared",self.get("shared"));return {};}
    if(name=="deserialize"){self.set("name",data.get("name"));self.set("version",data.get("version"));self.set("shared",data.get("shared"));return {};}
    throw std::logic_error("Unknown UniformGroupNode method");
}
}
JSValue nativeUniformGroupNodeConstructor(){
    static thread_local JSWeakValue weak;static thread_local bool initializing=false;
    if(const auto ctor=weak.lock();ctor&&ctor->isConstructible())return *ctor;
    JSModuleInitializationScope scope(initializing,"UniformGroupNode");auto state=JSValue::object(nullptr);const auto base=nativeNodeConstructor();
    auto ctor=JSValue::builtinConstructor([](const JSValue& newTarget,const Args& args,const Args& c)->JSValue{
        auto self=c[0].get("constructor").getPrototype().construct({"string"},newTarget);self.set("name",arg(args,0));self.set("shared",defaulted(args,1,false));self.set("order",defaulted(args,2,1));self.set("updateType",defaulted(args,3,nullptr));self.set("isUniformGroup",true);return self;
    },{state},"UniformGroupNode",1);state.set("constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;type.getter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return "UniformGroupNode";},{},"get type",0);type.configurable=true;ctor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"isCacheable",0},{"update",0},{"serialize",1},{"deserialize",1}}){JSPropertyDescriptor d;d.value=JSValue::function(method,{name},name,arity);d.writable=d.configurable=true;prototype.defineProperty(name,d);}
    ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;
}
namespace {JSWeakValue& uniformGroupBindings(){static thread_local JSWeakValue weak;return weak;}}
JSValue nativeUniformGroupNodeModule(){
    static thread_local JSWeakValue weak;static thread_local bool initializing=false;
    if(const auto module=weak.lock();module&&module->isObject())return *module;
    JSModuleInitializationScope scope(initializing,"UniformGroupNode module");auto bindings=JSValue::object(nullptr);const auto ctor=nativeUniformGroupNodeConstructor();
    bindings.set("default",ctor);
    for(const auto& [name,shared]:std::initializer_list<std::pair<const char*,bool>>{{"uniformGroup",false},{"sharedUniformGroup",true}}){
        bindings.set(name,JSValue::function([](const JSValue&,const Args& args,const Args& captures)->JSValue{
            const auto shared=captures[0].toBoolean();return captures[1].construct({arg(args,0),shared,defaulted(args,1,shared?0:1),defaulted(args,2,nullptr)});
        },{shared,ctor},name,1));
    }
    const auto updateTypes=nativeNodeConstantsBinding("NodeUpdateType");
    bindings.set("frameGroup",bindings.get("sharedUniformGroup").call({}, {"frame",0,updateTypes.get("FRAME")}));
    bindings.set("renderGroup",bindings.get("sharedUniformGroup").call({}, {"render",0,updateTypes.get("RENDER")}));
    bindings.set("objectGroup",bindings.get("uniformGroup").call({}, {"object",1,updateTypes.get("OBJECT")}));
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)module.set(name,bindings.get(name));
    bindings.retainForHeapLifetime();uniformGroupBindings()=JSWeakValue(bindings);
    module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeUniformGroupNodeBinding(const std::string& name){const auto keeper=nativeUniformGroupNodeModule();return uniformGroupBindings().lock()->get(name);}
JSValue nativeFrameGroup(){return nativeUniformGroupNodeBinding("frameGroup");}
JSValue nativeRenderGroup(){return nativeUniformGroupNodeBinding("renderGroup");}
JSValue nativeObjectGroup(){return nativeUniformGroupNodeBinding("objectGroup");}
UniformGroupNode::UniformGroupNode(const JSValue& name,const JSValue& shared,const JSValue& order,const JSValue& updateType):object_(constructor().construct({name,shared,order,updateType})){}
