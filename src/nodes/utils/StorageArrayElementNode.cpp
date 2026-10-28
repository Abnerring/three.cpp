#include "nodes/utils/StorageArrayElementNode.h"
#include "nodes/utils/ArrayElementNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
namespace {
using namespace UtilityNodeRuntime;
#include "StorageArrayElementNodeSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& self,const Args& args,const Args& c){
    const auto name=c[0],builder=arg(args,0);
    if(name.strictEquals("getMemberType")){
        const auto type=self.get("storageBufferNode").get("structTypeNode");
        if(type.toBoolean())return invoke(type,"getMemberType")({builder,arg(args,1)});return "void";
    }
    if(name.strictEquals("setup")){
        if(invoke(builder,"isAvailable")({"storageBuffer"}).strictEquals(false)){
            if(self.get("node").get("isPBO").strictEquals(true)){const auto setup=invoke(builder,"setupPBO");setup({self.get("node")});}
        }
        return c[1].getPrototype().get("setup",self).call(self,{builder});
    }
    const auto assign=invoke(builder,"isContextAssign")();JSValue snippet;
    if(invoke(builder,"isAvailable")({"storageBuffer"}).strictEquals(false)){
        if(self.get("node").get("isPBO").strictEquals(true)&&!assign.strictEquals(true)&&
           (self.get("node").get("value").get("isInstancedBufferAttribute").toBoolean()||!builder.get("shaderStage").strictEquals("compute"))){
            snippet=invoke(builder,"generatePBO")({self});
        }else snippet=invoke(self.get("node"),"build")({builder});
    }else snippet=c[1].getPrototype().get("generate",self).call(self,{builder});
    if(!assign.strictEquals(true)){const auto type=invoke(self,"getNodeType")({builder});snippet=invoke(builder,"format")({snippet,type,arg(args,1)});}
    return snippet;
}
}
JSValue nativeStorageArrayElementNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"StorageArrayElementNode module");
    const auto tsl=nativeTSLBase(),base=nativeArrayElementNodeConstructor();auto holder=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& c)->JSValue{auto self=superConstruct(target,c,{arg(args,0),arg(args,1)});self.set("isStorageArrayElementNode",true);return self;},{holder},"StorageArrayElementNode",2);
    holder.set("constructor",ctor);ctor.setPrototype(base);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(StorageArrayElementNodeSource::StorageArrayElementNode);
    JSPropertyDescriptor type;type.configurable=true;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "StorageArrayElementNode";},{},"get type",0);type.getter->setFunctionSourceText(StorageArrayElementNodeSource::get_type);ctor.defineProperty("type",type);
    JSPropertyDescriptor storage;storage.configurable=true;storage.setter=JSValue::function(+[](const JSValue& receiver,const Args& args,const Args&)->JSValue{auto self=receiver;self.set("node",arg(args,0));return {};},{},"set storageBufferNode",1);storage.setter->setFunctionSourceText(StorageArrayElementNodeSource::set_storageBufferNode);storage.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{return self.get("node");},{},"get storageBufferNode",0);storage.getter->setFunctionSourceText(StorageArrayElementNodeSource::get_storageBufferNode);p.defineProperty("storageBufferNode",storage);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"getMemberType",2},{"setup",1},{"generate",2}}){JSPropertyDescriptor d;d.writable=d.configurable=true;d.value=JSValue::function(method,{name,p},name,arity);d.value->setFunctionSourceText(StorageArrayElementNodeSource::get(name));p.defineProperty(name,d);}
    own(bindings,"default",ctor);const auto proxy=nativeTSLBaseBinding("nodeProxy").call({}, {ctor});own(bindings,"storageElement",invoke(proxy,"setParameterLength")({2}));
    auto module=JSValue::object();for(const auto* name:{"default","storageElement"})own(module,name,bindings.get(name));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeStorageArrayElementNodeBinding(const std::string& name){const auto keeper=nativeStorageArrayElementNodeModule();return lexical().lock()->get(name);}
JSValue nativeStorageArrayElementNodeConstructor(){return nativeStorageArrayElementNodeBinding("default");}
std::string StorageArrayElementNode::memberType(bool hasStructType,const std::string& answer){return hasStructType?answer:kMissingMemberType;}
bool StorageArrayElementNode::familiesAgreeOnMissingMember(){return std::string(kMissingMemberType)==kMemberNodeMissingType;}
bool StorageArrayElementNode::formats(bool assign){return !assign;}
bool StorageArrayElementNode::usesPboFallback(bool available,bool pbo,bool assign,bool instanced,bool compute){return !available&&pbo&&!assign&&(instanced||!compute);}
