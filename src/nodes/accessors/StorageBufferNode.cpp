#include "nodes/accessors/StorageBufferNode.h"
#include "nodes/accessors/BufferNode.h"
#include "nodes/accessors/BufferAttributeNode.h"
#include "nodes/utils/StorageArrayElementNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/tsl/TSLBase.h"
#include "core/JSGlobal.h"
namespace {
using namespace UtilityNodeRuntime;
#include "StorageBufferNodeSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& receiver,const Args& args,const Args& c){
    auto self=receiver;const auto name=c[0],imports=c[2],builder=arg(args,0);
    if(name.strictEquals("getHash")){
        JSValue id;
        if(self.get("bufferCount").strictEquals(0)){
            const auto get=invoke(builder.get("globalCache"),"getData");auto data=get({self.get("value")});
            if(data.isUndefined()){data=JSValue::object();own(data,"node",self);const auto set=invoke(builder.get("globalCache"),"setData");set({self.get("value"),data});}
            id=data.get("node").get("id");
        }else id=self.get("id");
        return nativeGlobalBinding("String").call({}, {id});
    }
    if(name.strictEquals("getInputType"))return self.get("value").get("isIndirectStorageBufferAttribute").toBoolean()?"indirectStorageBuffer":"storageBuffer";
    if(name.strictEquals("element"))return imports.get("storageElement").call({}, {self,arg(args,0)});
    if(name.strictEquals("setPBO")){self.set("isPBO",arg(args,0));return self;}
    if(name.strictEquals("getPBO"))return self.get("isPBO");
    if(name.strictEquals("setAccess")){self.set("access",arg(args,0));return self;}
    if(name.strictEquals("toReadOnly")){const auto set=invoke(self,"setAccess");return set({imports.get("NodeAccess").get("READ_ONLY")});}
    if(name.strictEquals("setAtomic")){self.set("isAtomic",arg(args,0));return self;}
    if(name.strictEquals("toAtomic"))return invoke(self,"setAtomic")({true});
    if(name.strictEquals("getAttributeData")){
        if(self.get("_attribute").isNull()){
            const auto factory=imports.get("bufferAttribute");self.set("_attribute",factory.call({}, {self.get("value")}));
            const auto varying=imports.get("varying");self.set("_varying",varying.call({}, {self.get("_attribute")}));
        }
        auto result=JSValue::object();own(result,"attribute",self.get("_attribute"));own(result,"varying",self.get("_varying"));return result;
    }
    if(name.strictEquals("generateNodeType")){
        if(!self.get("structTypeNode").isNull())return invoke(self.get("structTypeNode"),"getNodeType")({builder});
        return c[1].getPrototype().get("generateNodeType",self).call(self,{builder});
    }
    if(name.strictEquals("getMemberType")){
        if(!self.get("structTypeNode").isNull())return invoke(self.get("structTypeNode"),"getMemberType")({builder,arg(args,1)});return "void";
    }
    if(!self.get("structTypeNode").isNull())invoke(self.get("structTypeNode"),"build")({builder});
    if(invoke(builder,"isAvailable")({"storageBuffer"}).toBoolean()||invoke(builder,"isAvailable")({"indirectStorageBuffer"}).toBoolean())return c[1].getPrototype().get("generate",self).call(self,{builder});
    const auto data=invoke(self,"getAttributeData")(),attribute=data.get("attribute"),varying=data.get("varying");
    const auto output=invoke(varying,"build")({builder});invoke(builder,"registerTransform")({output,attribute});
    const auto format=invoke(builder,"format");const auto attributeType=invoke(attribute,"getNodeType")({builder}),type=invoke(self,"getNodeType")({builder});return format({output,attributeType,type});
}
}
JSValue nativeStorageBufferNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"StorageBufferNode module");
    const auto buffer=nativeBufferNodeModule(),attribute=nativeBufferAttributeNodeModule(),tsl=nativeTSLBase(),element=nativeStorageArrayElementNodeModule(),access=nativeNodeConstantsBinding("NodeAccess"),utils=nativeNodeUtils();
    const auto base=nativeBufferNodeBinding("default");auto imports=JSValue::object(nullptr),holder=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
    own(imports,"bufferAttribute",nativeBufferAttributeNodeBinding("bufferAttribute"));own(imports,"varying",nativeTSLBaseBinding("varying"));own(imports,"storageElement",nativeStorageArrayElementNodeBinding("storageElement"));own(imports,"NodeAccess",access);own(imports,"getTypeFromLength",nativeNodeUtilsBinding("getTypeFromLength"));
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& c)->JSValue{
        const auto value=arg(args,0),type=arg(args,1,nullptr),imports=c[1];auto count=arg(args,2,0),structType=JSValue(nullptr);JSValue nodeType;
        if(type.toBoolean()&&type.get("isStructTypeNode").toBoolean()){
            nodeType="struct";structType=type;if(value.get("isStorageBufferAttribute").toBoolean()||value.get("isStorageInstancedBufferAttribute").toBoolean())count=value.get("count");
        }else if(type.isNull()&&(value.get("isStorageBufferAttribute").toBoolean()||value.get("isStorageInstancedBufferAttribute").toBoolean())){
            const auto get=imports.get("getTypeFromLength");nodeType=get.call({}, {value.get("itemSize")});count=value.get("count");
        }else nodeType=type;
        auto self=superConstruct(target,c,{value,nodeType,count});self.set("isStorageBufferNode",true);self.set("structTypeNode",structType);self.set("access",imports.get("NodeAccess").get("READ_WRITE"));self.set("isAtomic",false);self.set("isPBO",false);self.set("_attribute",nullptr);self.set("_varying",nullptr);self.set("global",true);
        if(!value.get("isStorageBufferAttribute").strictEquals(true)&&!value.get("isStorageInstancedBufferAttribute").strictEquals(true)){
            auto mutableValue=value;if(value.get("isInstancedBufferAttribute").toBoolean())mutableValue.set("isStorageInstancedBufferAttribute",true);else mutableValue.set("isStorageBufferAttribute",true);
        }return self;
    },{holder,imports},"StorageBufferNode",1);holder.set("constructor",ctor);ctor.setPrototype(base);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(StorageBufferNodeSource::StorageBufferNode);
    JSPropertyDescriptor type;type.configurable=true;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "StorageBufferNode";},{},"get type",0);type.getter->setFunctionSourceText(StorageBufferNodeSource::get_type);ctor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"getHash",1},{"getInputType",0},{"element",1},{"setPBO",1},{"getPBO",0},{"setAccess",1},{"toReadOnly",0},{"setAtomic",1},{"toAtomic",0},{"getAttributeData",0},{"generateNodeType",1},{"getMemberType",2},{"generate",1}}){JSPropertyDescriptor d;d.writable=d.configurable=true;d.value=JSValue::function(method,{name,p,imports},name,arity);d.value->setFunctionSourceText(StorageBufferNodeSource::get(name));p.defineProperty(name,d);}
    own(bindings,"default",ctor);auto factory=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{return c[0].construct({arg(args,0),arg(args,1,nullptr),arg(args,2,0)});},{ctor},"storage",1);factory.setFunctionSourceText(StorageBufferNodeSource::storage);own(bindings,"storage",factory);
    auto module=JSValue::object();for(const auto* name:{"default","storage"})own(module,name,bindings.get(name));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeStorageBufferNodeBinding(const std::string& name){const auto keeper=nativeStorageBufferNodeModule();return lexical().lock()->get(name);}
JSValue nativeStorageBufferNodeConstructor(){return nativeStorageBufferNodeBinding("default");}
